#include "domain/Project.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <random>
#include <sstream>
#include <unordered_set>

namespace pms {
std::string newId() {
    static thread_local std::mt19937_64 rng(std::random_device{}());
    std::uint64_t a = rng(), b = rng();
    a = (a & ~0xf000ULL) | 0x4000ULL;
    b = (b & 0x3fffffffffffffffULL) | 0x8000000000000000ULL;
    std::ostringstream s;
    s << std::hex << std::setfill('0') << std::setw(8) << (a >> 32) << '-' << std::setw(4)
      << ((a >> 16) & 0xffff) << '-' << std::setw(4) << (a & 0xffff) << '-' << std::setw(4)
      << (b >> 48) << '-' << std::setw(12) << (b & 0xffffffffffffULL);
    return s.str();
}
Project makeProject() {
    Project p;
    p.id = newId();
    p.tracks = {{newId(), "V1", TrackType::Video, 0},
                {newId(), "V2", TrackType::Video, 1},
                {newId(), "A1", TrackType::Audio, 0},
                {newId(), "A2", TrackType::Audio, 1}};
    return p;
}
Time Project::duration() const {
    Time d = 0;
    for (const auto &c : clips)
        d = std::max(d, c.end());
    return d;
}
template <class T> const T &lookup(const std::vector<T> &v, const std::string &id) {
    auto i = std::find_if(v.begin(), v.end(), [&](const auto &x) { return x.id == id; });
    if (i == v.end())
        throw DomainError(Error::MissingItem);
    return *i;
}
const Asset &Project::asset(const std::string &id) const {
    return lookup(assets, id);
}
const Track &Project::track(const std::string &id) const {
    return lookup(tracks, id);
}
const Clip &Project::clip(const std::string &id) const {
    return lookup(clips, id);
}
Time frameTime(std::int64_t i, const Settings &s) {
    return (i * Second * s.fpsDen + s.fpsNum / 2) / s.fpsNum;
}
std::int64_t frameIndex(Time t, const Settings &s) {
    return (t * s.fpsNum + Second * s.fpsDen / 2) / (Second * s.fpsDen);
}
Time snapToFrame(Time t, const Settings &s) {
    return frameTime(frameIndex(t, s), s);
}
double gainAt(const Clip &c, Time sourceTime) {
    if (c.muted)
        return 0;
    double v = c.gain;
    if (c.fadeIn > 0)
        v *= std::clamp(double(sourceTime - c.envelopeIn) / double(c.fadeIn), 0.0, 1.0);
    if (c.fadeOut > 0)
        v *= std::clamp(double(c.envelopeOut - sourceTime) / double(c.fadeOut), 0.0, 1.0);
    return v;
}
void Project::validate() const {
    if (schemaVersion != 2)
        throw DomainError(Error::UnsupportedSchema);
    if (id.empty() || settings.width < 16 || settings.height < 16 || settings.width > 3840 ||
        settings.height > 3840 || settings.width % 2 || settings.height % 2 ||
        settings.fpsNum < 1 || settings.fpsNum > 60000 || settings.fpsDen < 1 ||
        settings.fpsDen > 1001 || double(settings.fpsNum) / settings.fpsDen > 60 ||
        double(settings.fpsNum) / settings.fpsDen < 1)
        throw DomainError(Error::InvalidProject);
    if (assets.size() > 10000 || clips.size() > 10000 || tracks.empty() || tracks.size() > 32)
        throw DomainError(Error::LimitExceeded);
    std::unordered_set<std::string> ids;
    auto unique = [&](const std::string &s) {
        if (s.empty() || s.size() > 128 || !ids.insert(s).second)
            throw DomainError(Error::InvalidProject);
    };
    for (const auto &a : assets) {
        unique(a.id);
        if ((a.kind != AssetKind::Media && a.kind != AssetKind::Image &&
             a.kind != AssetKind::Title) ||
            (a.kind != AssetKind::Title && a.path.empty()) || a.path.size() > 32768 ||
            a.duration <= 0 || a.duration > MaxTime || (a.videoStream < 0 && a.audioStream < 0) ||
            a.videoStream > 1024 || a.audioStream > 1024 || a.width < 0 || a.height < 0 ||
            a.width > 8192 || a.height > 8192 || a.startTime < -MaxTime || a.startTime > MaxTime ||
            (a.videoStream >= 0 && (a.width == 0 || a.height == 0)) ||
            (a.kind != AssetKind::Media && (a.videoStream != 0 || a.audioStream >= 0)))
            throw DomainError(Error::InvalidProject);
    }
    std::unordered_set<std::string> orders;
    for (const auto &t : tracks) {
        unique(t.id);
        if (t.order < 0 || t.order > 31 ||
            !orders.insert(std::to_string(int(t.type)) + ":" + std::to_string(t.order)).second)
            throw DomainError(Error::InvalidProject);
    }
    for (const auto &c : clips) {
        unique(c.id);
        const auto &a = asset(c.assetId);
        const auto &t = track(c.trackId);
        if ((t.type == TrackType::Video && a.videoStream < 0) ||
            (t.type == TrackType::Audio && a.audioStream < 0))
            throw DomainError(Error::WrongTrack);
        if (c.start < 0 || c.start > MaxTime || c.sourceIn < 0 || c.sourceOut <= c.sourceIn ||
            c.sourceOut > a.duration || c.end() > MaxTime || !std::isfinite(c.gain) || c.gain < 0 ||
            c.gain > 4 || c.envelopeIn < 0 || c.envelopeOut > a.duration ||
            c.envelopeOut < c.envelopeIn || c.fadeIn < 0 || c.fadeOut < 0 ||
            c.fadeIn > c.envelopeOut - c.envelopeIn || c.fadeOut > c.envelopeOut - c.envelopeIn)
            throw DomainError(Error::InvalidRange);
        const auto &v = c.visual;
        auto range = [](double x, double lo, double hi) {
            return std::isfinite(x) && x >= lo && x <= hi;
        };
        if (!range(v.brightness, -1, 1) || !range(v.contrast, 0, 3) || !range(v.saturation, 0, 3) ||
            !range(v.hue, -180, 180) || v.envelopeIn < 0 || v.envelopeOut < v.envelopeIn ||
            v.envelopeOut > a.duration || v.fadeIn < 0 || v.fadeOut < 0 ||
            v.fadeIn > v.envelopeOut - v.envelopeIn || v.fadeOut > v.envelopeOut - v.envelopeIn ||
            (v.transition != Transition::Fade && v.transition != Transition::Wipe &&
             v.transition != Transition::Slide))
            throw DomainError(Error::InvalidRange);
        const auto &z = c.title;
        if (z.text.size() > 8192 || z.font.empty() || z.font.size() > 256 || z.color.size() != 7 ||
            z.color[0] != '#' ||
            !std::all_of(z.color.begin() + 1, z.color.end(),
                         [](unsigned char ch) { return std::isxdigit(ch); }) ||
            !range(z.x, 0, 1) || !range(z.y, 0, 1) || !range(z.width, .05, 1) ||
            !range(z.size, 8, 500))
            throw DomainError(Error::InvalidRange);
    }
    // Sorting avoids quadratic validation on long projects.
    std::vector<const Clip *> sorted;
    for (const auto &c : clips)
        sorted.push_back(&c);
    std::sort(sorted.begin(), sorted.end(), [](auto a, auto b) {
        return a->trackId == b->trackId ? a->start < b->start : a->trackId < b->trackId;
    });
    for (std::size_t i = 1; i < sorted.size(); ++i)
        if (sorted[i - 1]->trackId == sorted[i]->trackId && sorted[i - 1]->end() > sorted[i]->start)
            throw DomainError(Error::Overlap);
}
} // namespace pms
