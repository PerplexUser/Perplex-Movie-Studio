#include "persistence/ProjectStore.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <climits>
#include <cmath>
#include <limits>
namespace pms {
static QString qs(const std::string &s) {
    return QString::fromStdString(s);
}
static qint64 integer(const QJsonObject &o, const char *key, qint64 fallback = 0,
                      bool required = true) {
    if (!o.contains(key) && !required)
        return fallback;
    auto v = o[key];
    double d = v.toDouble(std::numeric_limits<double>::quiet_NaN());
    if (!v.isDouble() || !std::isfinite(d) || std::trunc(d) != d ||
        std::abs(d) > 9007199254740991.0)
        throw DomainError(Error::InvalidProject);
    return qint64(d);
}
static int small(const QJsonObject &o, const char *key, int fallback = 0, bool required = true) {
    auto v = integer(o, key, fallback, required);
    if (v < INT_MIN || v > INT_MAX)
        throw DomainError(Error::InvalidProject);
    return int(v);
}
static std::string string(const QJsonObject &o, const char *key) {
    if (!o[key].isString() || o[key].toString().size() > 32768)
        throw DomainError(Error::InvalidProject);
    return o[key].toString().toStdString();
}
static bool boolean(const QJsonObject &o, const char *key) {
    if (!o[key].isBool())
        throw DomainError(Error::InvalidProject);
    return o[key].toBool();
}
static double number(const QJsonObject &o, const char *k) {
    if (!o[k].isDouble())
        throw DomainError(Error::InvalidProject);
    return o[k].toDouble();
}
QJsonObject ProjectStore::encode(const Project &p, const QString &path) {
    p.validate();
    QDir base = QFileInfo(path).absoluteDir();
    QJsonArray assets, tracks, clips;
    for (const auto &a : p.assets)
        assets.append(QJsonObject{
            {"id", qs(a.id)},
            {"kind", int(a.kind)},
            {"path", a.kind == AssetKind::Title ? QString() : base.relativeFilePath(qs(a.path))},
            {"absolutePath", qs(a.path)},
            {"name", qs(a.name)},
            {"container", qs(a.container)},
            {"videoCodec", qs(a.videoCodec)},
            {"audioCodec", qs(a.audioCodec)},
            {"duration", qint64(a.duration)},
            {"startTime", qint64(a.startTime)},
            {"width", a.width},
            {"height", a.height},
            {"videoStream", a.videoStream},
            {"audioStream", a.audioStream},
            {"sampleRate", a.sampleRate},
            {"channels", a.channels},
            {"rotation", a.rotation},
            {"fpsNum", a.fpsNum},
            {"fpsDen", a.fpsDen}});
    for (const auto &t : p.tracks)
        tracks.append(QJsonObject{{"id", qs(t.id)},
                                  {"name", qs(t.name)},
                                  {"type", t.type == TrackType::Video ? "video" : "audio"},
                                  {"order", t.order},
                                  {"mute", t.muted},
                                  {"lock", t.locked}});
    for (const auto &c : p.clips)
        clips.append(
            QJsonObject{{"id", qs(c.id)},
                        {"assetId", qs(c.assetId)},
                        {"trackId", qs(c.trackId)},
                        {"timelineStart", qint64(c.start)},
                        {"sourceIn", qint64(c.sourceIn)},
                        {"sourceOut", qint64(c.sourceOut)},
                        {"gain", c.gain},
                        {"mute", c.muted},
                        {"envelopeIn", qint64(c.envelopeIn)},
                        {"envelopeOut", qint64(c.envelopeOut)},
                        {"fadeIn", qint64(c.fadeIn)},
                        {"fadeOut", qint64(c.fadeOut)},
                        {"visual", QJsonObject{{"brightness", c.visual.brightness},
                                               {"contrast", c.visual.contrast},
                                               {"saturation", c.visual.saturation},
                                               {"hue", c.visual.hue},
                                               {"envelopeIn", qint64(c.visual.envelopeIn)},
                                               {"envelopeOut", qint64(c.visual.envelopeOut)},
                                               {"fadeIn", qint64(c.visual.fadeIn)},
                                               {"fadeOut", qint64(c.visual.fadeOut)},
                                               {"transition", int(c.visual.transition)}}},
                        {"title", QJsonObject{{"text", qs(c.title.text)},
                                              {"font", qs(c.title.font)},
                                              {"color", qs(c.title.color)},
                                              {"x", c.title.x},
                                              {"y", c.title.y},
                                              {"width", c.title.width},
                                              {"size", c.title.size},
                                              {"bold", c.title.bold}}},
                        {"effects", QJsonArray{}}});
    return {{"schemaVersion", p.schemaVersion},
            {"id", qs(p.id)},
            {"name", qs(p.name)},
            {"settings", QJsonObject{{"width", p.settings.width},
                                     {"height", p.settings.height},
                                     {"fpsNum", p.settings.fpsNum},
                                     {"fpsDen", p.settings.fpsDen}}},
            {"assets", assets},
            {"tracks", tracks},
            {"clips", clips}};
}
Project ProjectStore::decode(const QJsonObject &o, const QString &path) {
    const int schema = small(o, "schemaVersion");
    if (schema != 1 && schema != 2)
        throw DomainError(Error::UnsupportedSchema);
    Project p;
    p.id = string(o, "id");
    p.name = string(o, "name");
    auto s = o["settings"].toObject();
    p.settings = {small(s, "width"), small(s, "height"), small(s, "fpsNum"), small(s, "fpsDen")};
    for (auto key : {"assets", "tracks", "clips"})
        if (!o[key].isArray() || o[key].toArray().size() > 10000)
            throw DomainError(Error::InvalidProject);
    QDir base = QFileInfo(path).absoluteDir();
    for (const auto v : o["assets"].toArray()) {
        auto a = v.toObject();
        Asset x;
        if (schema == 2)
            x.kind = AssetKind(small(a, "kind"));
        x.id = string(a, "id");
        x.name = string(a, "name");
        auto rel = qs(string(a, "path")), fallback = qs(string(a, "absolutePath"));
        QString resolved = QDir::cleanPath(base.absoluteFilePath(rel));
        if (!QFileInfo(resolved).isFile() && QFileInfo(fallback).isFile())
            resolved = fallback;
        x.path = x.kind == AssetKind::Title ? std::string() : resolved.toStdString();
        x.container = string(a, "container");
        x.videoCodec = string(a, "videoCodec");
        x.audioCodec = string(a, "audioCodec");
        x.duration = integer(a, "duration");
        x.startTime = integer(a, "startTime", 0, false);
        x.width = small(a, "width");
        x.height = small(a, "height");
        x.videoStream = small(a, "videoStream");
        x.audioStream = small(a, "audioStream");
        x.sampleRate = small(a, "sampleRate");
        x.channels = small(a, "channels");
        x.rotation = small(a, "rotation");
        x.fpsNum = small(a, "fpsNum");
        x.fpsDen = small(a, "fpsDen");
        p.assets.push_back(x);
    }
    for (const auto v : o["tracks"].toArray()) {
        auto t = v.toObject();
        auto type = string(t, "type");
        if (type != "video" && type != "audio")
            throw DomainError(Error::InvalidProject);
        p.tracks.push_back({string(t, "id"), string(t, "name"),
                            type == "video" ? TrackType::Video : TrackType::Audio,
                            small(t, "order"), boolean(t, "mute"), boolean(t, "lock")});
    }
    for (const auto v : o["clips"].toArray()) {
        auto c = v.toObject();
        Clip x;
        x.id = string(c, "id");
        x.assetId = string(c, "assetId");
        x.trackId = string(c, "trackId");
        x.start = integer(c, "timelineStart");
        x.sourceIn = integer(c, "sourceIn");
        x.sourceOut = integer(c, "sourceOut");
        if (!c["gain"].isDouble())
            throw DomainError(Error::InvalidProject);
        x.gain = c["gain"].toDouble();
        x.muted = boolean(c, "mute");
        x.envelopeIn = integer(c, "envelopeIn", x.sourceIn, false);
        x.envelopeOut = integer(c, "envelopeOut", x.sourceOut, false);
        x.fadeIn = integer(c, "fadeIn", 0, false);
        x.fadeOut = integer(c, "fadeOut", 0, false);
        if (c.contains("effects") && (!c["effects"].isArray() || !c["effects"].toArray().isEmpty()))
            throw DomainError(Error::UnsupportedSchema);
        x.visual.envelopeIn = x.sourceIn;
        x.visual.envelopeOut = x.sourceOut;
        if (schema == 2) {
            auto v = c["visual"].toObject(), t = c["title"].toObject();
            x.visual.brightness = number(v, "brightness");
            x.visual.contrast = number(v, "contrast");
            x.visual.saturation = number(v, "saturation");
            x.visual.hue = number(v, "hue");
            x.visual.envelopeIn = integer(v, "envelopeIn");
            x.visual.envelopeOut = integer(v, "envelopeOut");
            x.visual.fadeIn = integer(v, "fadeIn");
            x.visual.fadeOut = integer(v, "fadeOut");
            x.visual.transition = Transition(small(v, "transition"));
            x.title.text = string(t, "text");
            x.title.font = string(t, "font");
            x.title.color = string(t, "color");
            x.title.x = number(t, "x");
            x.title.y = number(t, "y");
            x.title.width = number(t, "width");
            x.title.size = number(t, "size");
            x.title.bold = boolean(t, "bold");
        }
        p.clips.push_back(x);
    }
    p.validate();
    return p;
}
void ProjectStore::save(const Project &p, const QString &path) {
    const QFileInfo target(path);
    for (const auto &a : p.assets) {
        if (a.kind == AssetKind::Title)
            continue;
        const QFileInfo source(qs(a.path));
        if (source.absoluteFilePath() == target.absoluteFilePath() ||
            (!target.canonicalFilePath().isEmpty() &&
             source.canonicalFilePath() == target.canonicalFilePath()))
            throw DomainError(Error::InvalidProject);
    }
    auto bytes = QJsonDocument(encode(p, path)).toJson(QJsonDocument::Indented);
    if (bytes.size() > 16 * 1024 * 1024)
        throw DomainError(Error::LimitExceeded);
    QSaveFile file(path);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
        throw std::runtime_error(file.errorString().toStdString());
}
Project ProjectStore::load(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error(file.errorString().toStdString());
    if (file.size() > 16 * 1024 * 1024)
        throw DomainError(Error::LimitExceeded);
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject())
        throw DomainError(Error::InvalidProject);
    return decode(doc.object(), path);
}
QStringList ProjectStore::missing(const Project &p) {
    QStringList list;
    for (const auto &a : p.assets)
        if (a.kind != AssetKind::Title && !QFileInfo(QString::fromStdString(a.path)).isFile())
            list << QString::fromStdString(a.id);
    return list;
}
} // namespace pms
