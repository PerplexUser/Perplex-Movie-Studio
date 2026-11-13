#include "commands/Editor.h"
#include <algorithm>
namespace pms {
static Clip &editable(Project &p, const std::string &id) {
    const auto &c = p.clip(id);
    if (p.track(c.trackId).locked)
        throw DomainError(Error::LockedTrack);
    return *std::find_if(p.clips.begin(), p.clips.end(), [&](auto &x) { return x.id == id; });
}
static void unlocked(const Project &p, const std::string &id) {
    if (p.track(id).locked)
        throw DomainError(Error::LockedTrack);
}
Editor::Editor(Project p) {
    reset(std::move(p));
}
void Editor::reset(Project p) {
    p.validate();
    current_ = std::move(p);
    saved_ = current_;
    undo_.clear();
    redo_.clear();
}
bool Editor::apply(const std::function<void(Project &)> &fn) {
    Project next = current_;
    fn(next);
    next.validate();
    if (next == current_)
        return false;
    undo_.push_back(current_);
    if (undo_.size() > 100)
        undo_.erase(undo_.begin());
    current_ = std::move(next);
    redo_.clear();
    return true;
}
void Editor::undo() {
    if (!canUndo())
        return;
    redo_.push_back(current_);
    current_ = std::move(undo_.back());
    undo_.pop_back();
}
void Editor::redo() {
    if (!canRedo())
        return;
    undo_.push_back(current_);
    current_ = std::move(redo_.back());
    redo_.pop_back();
}
std::string Editor::addAsset(Asset a) {
    for (const auto &x : current_.assets)
        if (a.kind != AssetKind::Title && x.path == a.path)
            return x.id;
    a.id = newId();
    apply([&](Project &p) { p.assets.push_back(a); });
    return a.id;
}
std::string Editor::addClip(const std::string &assetId, const std::string &trackId, Time start) {
    const auto &a = current_.asset(assetId);
    Clip c;
    c.id = newId();
    c.assetId = assetId;
    c.trackId = trackId;
    c.start = start;
    c.sourceOut = a.kind == AssetKind::Media ? a.duration : 5 * Second;
    c.envelopeOut = a.duration;
    apply([&](Project &p) {
        unlocked(p, trackId);
        p.clips.push_back(c);
    });
    return c.id;
}
std::string Editor::split(const std::string &id, Time at) {
    const std::string rightId = newId();
    apply([&](Project &p) {
        auto &c = editable(p, id);
        if (at <= c.start || at >= c.end())
            throw DomainError(Error::InvalidRange);
        Clip right = c;
        right.id = rightId;
        right.start = at;
        right.sourceIn += at - c.start;
        c.sourceOut = right.sourceIn;
        p.clips.push_back(right);
    });
    return rightId;
}
void Editor::move(const std::string &id, const std::string &trackId, Time start) {
    apply([&](Project &p) {
        unlocked(p, trackId);
        auto &c = editable(p, id);
        c.trackId = trackId;
        c.start = start;
    });
}
void Editor::trim(const std::string &id, Time in, Time out, Time start) {
    apply([&](Project &p) {
        auto &c = editable(p, id);
        c.visual.envelopeIn = in;
        c.visual.envelopeOut = out;
        c.visual.fadeIn = std::min(c.visual.fadeIn, out - in);
        c.visual.fadeOut = std::min(c.visual.fadeOut, out - in);
        c.sourceIn = in;
        c.sourceOut = out;
        c.start = start;
    });
}
void Editor::erase(const std::string &id) {
    apply([&](Project &p) {
        editable(p, id);
        std::erase_if(p.clips, [&](auto &c) { return c.id == id; });
    });
}
std::string Editor::duplicate(const std::string &id) {
    auto c = current_.clip(id);
    return paste(c, c.trackId, c.end());
}
std::string Editor::paste(Clip c, const std::string &trackId, Time start) {
    c.id = newId();
    c.start = start;
    c.trackId = trackId;
    apply([&](Project &p) {
        unlocked(p, trackId);
        p.clips.push_back(c);
    });
    return c.id;
}
void Editor::audio(const std::string &id, double gain, bool muted, Time in, Time out) {
    apply([&](Project &p) {
        auto &c = editable(p, id);
        c.gain = gain;
        c.muted = muted;
        if (c.fadeIn != in || c.fadeOut != out) {
            c.envelopeIn = c.sourceIn;
            c.envelopeOut = c.sourceOut;
            c.visual.envelopeOut = c.sourceOut;
        }
        c.fadeIn = in;
        c.fadeOut = out;
    });
}
void Editor::trackState(const std::string &id, bool muted, bool locked) {
    apply([&](Project &p) {
        for (auto &t : p.tracks)
            if (t.id == id) {
                if (t.locked && muted != t.muted)
                    throw DomainError(Error::LockedTrack);
                t.muted = muted;
                t.locked = locked;
                return;
            }
        throw DomainError(Error::MissingItem);
    });
}
void Editor::relink(const std::string &id, const Asset &replacement) {
    apply([&](Project &p) {
        for (auto &a : p.assets)
            if (a.id == id) {
                a = replacement;
                a.id = id;
                return;
            }
        throw DomainError(Error::MissingItem);
    });
}
void Editor::removeAsset(const std::string &id) {
    apply([&](Project &p) {
        p.asset(id);
        if (std::any_of(p.clips.begin(), p.clips.end(),
                        [&](const Clip &c) { return c.assetId == id; }))
            throw DomainError(Error::AssetInUse);
        std::erase_if(p.assets, [&](const Asset &a) { return a.id == id; });
    });
}
std::string Editor::addTitle(Time start, const std::string &text) {
    const auto id = newId();
    apply([&](Project &p) {
        Asset a;
        a.id = newId();
        a.kind = AssetKind::Title;
        a.name = text;
        a.duration = MaxTime;
        a.videoStream = 0;
        a.width = p.settings.width;
        a.height = p.settings.height;
        a.container = "title";
        a.videoCodec = "rgba";
        std::string tid;
        int order = 0;
        for (const auto &t : p.tracks)
            if (t.type == TrackType::Video) {
                order = std::max(order, t.order + 1);
                if (t.name == "T1") {
                    unlocked(p, t.id);
                    tid = t.id;
                }
            }
        if (tid.empty()) {
            tid = newId();
            p.tracks.push_back({tid, "T1", TrackType::Video, order});
        }
        Clip c;
        c.id = id;
        c.assetId = a.id;
        c.trackId = tid;
        c.start = start;
        c.sourceOut = 5 * Second;
        c.envelopeOut = c.sourceOut;
        c.visual.envelopeOut = c.sourceOut;
        c.title.text = text;
        p.assets.push_back(a);
        p.clips.push_back(c);
    });
    return id;
}
void Editor::visual(const std::string &id, Visual v) {
    apply([&](Project &p) {
        auto &c = editable(p, id);
        if (p.track(c.trackId).type != TrackType::Video)
            throw DomainError(Error::WrongTrack);
        if (v.fadeIn != c.visual.fadeIn || v.fadeOut != c.visual.fadeOut) {
            v.envelopeIn = c.sourceIn;
            v.envelopeOut = c.sourceOut;
        }
        c.visual = v;
    });
}
void Editor::title(const std::string &id, const Title &t) {
    apply([&](Project &p) {
        auto &c = editable(p, id);
        if (p.asset(c.assetId).kind != AssetKind::Title)
            throw DomainError(Error::WrongTrack);
        c.title = t;
    });
}
void Editor::settings(const Settings &s) {
    apply([&](Project &p) { p.settings = s; });
}
} // namespace pms
