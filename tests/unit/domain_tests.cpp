#include "commands/Editor.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x))                                                                                  \
            throw std::runtime_error("FAIL line " + std::to_string(__LINE__) + ": " #x);           \
        ++checks;                                                                                  \
    } while (false)
int checks = 0;
template <class F> void rejects(F fn, pms::Error expected) {
    bool caught = false;
    try {
        fn();
    } catch (const pms::DomainError &e) {
        CHECK(e.code == expected);
        caught = true;
    }
    CHECK(caught);
}
int main() {
    using namespace pms;
    try {
        Editor e;
        auto p = e.project();
        CHECK(p.tracks.size() == 4);
        CHECK(!e.dirty());
        Asset a;
        a.path = "C:/test/clip.mp4";
        a.duration = 10 * Second;
        a.videoStream = 0;
        a.audioStream = 1;
        a.width = 1920;
        a.height = 1080;
        auto aid = e.addAsset(a), tid = e.project().tracks[0].id;
        auto cid = e.addClip(aid, tid, 0);
        e.markSaved();
        auto rid = e.split(cid, 4 * Second);
        CHECK(e.project().clip(cid).duration() == 4 * Second);
        CHECK(e.project().clip(rid).duration() == 6 * Second);
        CHECK(e.project().clip(rid).sourceIn == 4 * Second);
        CHECK(e.dirty());
        e.undo();
        CHECK(!e.dirty());
        e.redo();
        auto before = e.project();
        rejects([&] { e.move(rid, tid, 3 * Second); }, Error::Overlap);
        CHECK(e.project() == before);
        e.trim(rid, 5 * Second, 9 * Second, 5 * Second);
        CHECK(e.project().clip(rid).duration() == 4 * Second);
        e.trackState(tid, false, true);
        before = e.project();
        rejects([&] { e.erase(cid); }, Error::LockedTrack);
        CHECK(e.project() == before);
        rejects([&] { e.addClip(aid, tid, 20 * Second); }, Error::LockedTrack);
        e.trackState(tid, false, false);
        e.erase(cid);
        CHECK(e.project().clips.size() == 1);
        e.undo();
        CHECK(e.project().clips.size() == 2);
        rejects([&] { e.split(cid, 0); }, Error::InvalidRange);
        rejects([&] { e.split(cid, 4 * Second); }, Error::InvalidRange);
        rejects([&] { e.trim(cid, -1, 4 * Second, 0); }, Error::InvalidRange);
        rejects([&] { e.audio(cid, std::numeric_limits<double>::quiet_NaN(), false, 0, 0); },
                Error::InvalidRange);
        auto p2 = makeProject();
        p2.schemaVersion = 3;
        rejects([&] { p2.validate(); }, Error::UnsupportedSchema);
        p2 = makeProject();
        p2.tracks[1].order = 0;
        rejects([&] { p2.validate(); }, Error::InvalidProject);
        Settings s;
        s.fpsNum = 30000;
        s.fpsDen = 1001;
        CHECK(frameTime(30000, s) == 1001 * Second);
        for (int i = 0; i < 100000; i += 17)
            CHECK(frameIndex(frameTime(i, s), s) == i);
        Editor f;
        aid = f.addAsset(a);
        tid = f.project().tracks[0].id;
        cid = f.addClip(aid, tid, 0);
        f.audio(cid, 0.8, false, 5 * Second, 4 * Second);
        auto original = f.project().clip(cid);
        rid = f.split(cid, 2 * Second);
        auto right = f.project().clip(rid);
        CHECK(std::abs(gainAt(original, 3 * Second) - gainAt(right, 3 * Second)) < 1e-12);
        for (int i = 0; i < 110; i++)
            f.move(rid, tid, (12 + i) * Second);
        int n = 0;
        while (f.canUndo()) {
            f.undo();
            ++n;
        }
        CHECK(n == 100);
        std::cout << "PASS " << checks << " domain assertions\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
