#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace pms {
using Time = std::int64_t;
inline constexpr Time Second = 1'000'000;
inline constexpr Time MaxTime = 24 * 60 * 60 * Second;
enum class Error {
    InvalidProject,
    UnsupportedSchema,
    MissingItem,
    LockedTrack,
    Overlap,
    InvalidRange,
    WrongTrack,
    EmptyProject,
    LimitExceeded,
    AssetInUse
};
struct DomainError : std::runtime_error {
    Error code;
    explicit DomainError(Error c) : std::runtime_error("Invalid editing operation"), code(c) {}
};
enum class TrackType { Video, Audio };
enum class AssetKind { Media, Image, Title };
enum class Transition { Fade, Wipe, Slide };
struct Visual {
    double brightness = 0, contrast = 1, saturation = 1, hue = 0;
    Time envelopeIn = 0, envelopeOut = 0, fadeIn = 0, fadeOut = 0;
    Transition transition = Transition::Fade;
    bool operator==(const Visual &) const = default;
};
struct Title {
    std::string text = "Text", font = "Segoe UI", color = "#ffffff";
    double x = .5, y = .8, width = .9, size = 72;
    bool bold = true;
    bool operator==(const Title &) const = default;
};
struct Settings {
    int width = 1920, height = 1080, fpsNum = 30, fpsDen = 1;
    bool operator==(const Settings &) const = default;
};
struct Asset {
    AssetKind kind = AssetKind::Media;
    std::string id, path, name, container, videoCodec, audioCodec;
    Time duration = 0, startTime = 0;
    int width = 0, height = 0, videoStream = -1, audioStream = -1;
    int sampleRate = 0, channels = 0, rotation = 0;
    int fpsNum = 0, fpsDen = 1;
    bool operator==(const Asset &) const = default;
};
struct Track {
    std::string id, name;
    TrackType type = TrackType::Video;
    int order = 0;
    bool muted = false, locked = false;
    bool operator==(const Track &) const = default;
};
struct Clip {
    std::string id, assetId, trackId;
    Time start = 0, sourceIn = 0, sourceOut = 0;
    double gain = 1.0;
    bool muted = false;
    // Fades use absolute source-time anchors: splitting preserves the original envelope.
    Time envelopeIn = 0, envelopeOut = 0, fadeIn = 0, fadeOut = 0;
    Visual visual;
    Title title;
    Time duration() const {
        return sourceOut - sourceIn;
    }
    Time end() const {
        return start + duration();
    }
    bool operator==(const Clip &) const = default;
};
struct Project {
    int schemaVersion = 2;
    std::string id, name = "Untitled";
    Settings settings;
    std::vector<Asset> assets;
    std::vector<Track> tracks;
    std::vector<Clip> clips;
    Time duration() const;
    const Asset &asset(const std::string &) const;
    const Track &track(const std::string &) const;
    const Clip &clip(const std::string &) const;
    void validate() const;
    bool operator==(const Project &) const = default;
};
std::string newId();
Project makeProject();
Time frameTime(std::int64_t index, const Settings &);
std::int64_t frameIndex(Time time, const Settings &);
Time snapToFrame(Time time, const Settings &);
double gainAt(const Clip &, Time sourceTime);
} // namespace pms
