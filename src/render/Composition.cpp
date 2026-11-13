#include "render/Composition.h"
#include "media/MediaTools.h"
#include <QFileInfo>
#include <QHash>
#include <algorithm>
namespace pms {
Composition compose(const Project &p, const RenderProfile &profile,
                    const QHash<QString, QString> &visualAssets) {
    p.validate();
    if (p.clips.empty())
        throw DomainError(Error::EmptyProject);
    if (!QStringList{"mp4", "mkv", "mp3", "wav"}.contains(profile.format) || profile.width < 16 ||
        profile.height < 16 || profile.width > 3840 || profile.height > 3840 || profile.width % 2 ||
        profile.height % 2 || profile.fps < 1 || profile.fps > 60)
        throw DomainError(Error::InvalidProject);
    Composition plan;
    plan.duration = p.duration();
    QString d = seconds(plan.duration);
    const bool video = profile.format == "mp4" || profile.format == "mkv";
    QHash<QString, int> indices;
    std::vector<const Clip *> clips;
    for (const auto &c : p.clips)
        clips.push_back(&c);
    std::stable_sort(clips.begin(), clips.end(), [&](auto a, auto b) {
        auto ta = p.track(a->trackId), tb = p.track(b->trackId);
        return ta.order == tb.order ? a->start < b->start : ta.order < tb.order;
    });
    for (const auto *c : clips) {
        const auto &a = p.asset(c->assetId);
        if (!video && a.audioStream < 0)
            continue;
        auto id = QString::fromStdString(a.kind == AssetKind::Title ? c->id : a.id);
        if (!indices.contains(id)) {
            if (indices.size() >= 128)
                throw DomainError(Error::LimitExceeded);
            auto path = a.kind == AssetKind::Media ? QString::fromStdString(a.path)
                                                   : visualAssets.value(id);
            if (!QFileInfo(path).isFile())
                throw DomainError(Error::MissingItem);
            indices[id] = indices.size();
            plan.inputArgs << "-threads" << "2" << "-hwaccel" << "none";
            if (a.kind != AssetKind::Media)
                plan.inputArgs << "-stream_loop" << "-1" << "-f" << "rawvideo" << "-pixel_format"
                               << "rgba" << "-video_size"
                               << QString("%1x%2").arg(profile.width).arg(profile.height)
                               << "-framerate" << QString::number(profile.fps);
            plan.inputArgs += localInput(path);
        }
    }
    QStringList f;
    QString vbase = "vb0";
    int vn = 0, an = 0;
    if (video)
        f << QString("color=c=black:s=%1x%2:r=%3:d=%4,format=yuv420p[%5]")
                 .arg(profile.width)
                 .arg(profile.height)
                 .arg(profile.fps)
                 .arg(d, vbase);
    f << QString("anullsrc=r=48000:cl=stereo:d=%1[ab0]").arg(d);
    QStringList audio{"[ab0]"};
    for (const auto *c : clips) {
        const auto &a = p.asset(c->assetId);
        const auto &t = p.track(c->trackId);
        int i = indices.value(QString::fromStdString(a.kind == AssetKind::Title ? c->id : a.id));
        if (video && t.type == TrackType::Video && a.videoStream >= 0) {
            QString label = QString("vc%1").arg(vn), next = QString("vb%1").arg(vn + 1);
            const auto &v = c->visual;
            auto num = [](double n) { return QString::number(n, 'f', 8); };
            QString chain =
                QString("[%1:%2]settb=AVTB,setpts=PTS-(%3)/TB,trim=start=%4:end=%5,fps=%6,")
                    .arg(i)
                    .arg(a.kind == AssetKind::Media ? a.videoStream : 0)
                    .arg(seconds(a.kind == AssetKind::Media ? a.startTime : -c->sourceIn),
                         seconds(c->sourceIn), seconds(c->sourceOut))
                    .arg(profile.fps);
            chain += QString("scale=%1:%2:force_original_aspect_ratio=decrease:force_divisible_by="
                             "2,setsar=1,format=rgba,pad=%1:%2:(ow-iw)/2:(oh-ih)/2:color=%3")
                         .arg(profile.width)
                         .arg(profile.height)
                         .arg(a.kind == AssetKind::Media ? "black" : "black@0");
            if (v.brightness != 0 || v.contrast != 1) {
                auto value = QString("clip((val-128)*%1+128+%2*255,0,255)")
                                 .arg(num(v.contrast), num(v.brightness));
                chain += QString(",lutrgb=r='%1':g='%1':b='%1'").arg(value);
            }
            if (v.saturation != 1 || v.hue != 0)
                chain += QString(",format=yuva444p,hue=h=%1:s=%2,format=rgba")
                             .arg(num(v.hue), num(v.saturation));
            if (v.transition == Transition::Fade) {
                if (v.fadeIn > 0)
                    chain += QString(",fade=t=in:st=%1:d=%2:alpha=1")
                                 .arg(seconds(v.envelopeIn), seconds(v.fadeIn));
                if (v.fadeOut > 0)
                    chain += QString(",fade=t=out:st=%1:d=%2:alpha=1")
                                 .arg(seconds(v.envelopeOut - v.fadeOut), seconds(v.fadeOut));
            } else if (v.transition == Transition::Wipe && (v.fadeIn || v.fadeOut)) {
                QString alpha = "alpha(X,Y)";
                if (v.fadeIn)
                    alpha += QString("*lt(X,W*clip((T-%1)/%2,0,1))")
                                 .arg(seconds(v.envelopeIn), seconds(v.fadeIn));
                if (v.fadeOut)
                    alpha += QString("*lt(X,W*clip((%1-T)/%2,0,1))")
                                 .arg(seconds(v.envelopeOut), seconds(v.fadeOut));
                chain += QString(",geq=r='r(X,Y)':g='g(X,Y)':b='b(X,Y)':a='%1'").arg(alpha);
            }
            chain += QString(",setpts=PTS-(%1)/TB+(%2)/TB[%3]")
                         .arg(seconds(c->sourceIn), seconds(c->start), label);
            f << chain;
            QString x = "0";
            if (v.transition == Transition::Slide) {
                auto clock = QString("(t+%1-%2)").arg(seconds(c->sourceIn), seconds(c->start));
                if (v.fadeIn)
                    x += QString("+W*(1-clip((%1-%2)/%3,0,1))")
                             .arg(clock, seconds(v.envelopeIn), seconds(v.fadeIn));
                if (v.fadeOut)
                    x += QString("-W*clip((%1-%2)/%3,0,1)")
                             .arg(clock, seconds(v.envelopeOut - v.fadeOut), seconds(v.fadeOut));
            }
            f << QString("[%1][%2]overlay=x='%3':y=0:format=yuv420:eof_action=pass:repeatlast=0:"
                         "shortest=0:enable='gte(t,%4)*lt(t,%5)'[%6]")
                     .arg(vbase, label, x, seconds(c->start), seconds(c->end()), next);
            vbase = next;
            ++vn;
        }
        if (a.audioStream >= 0 && !c->muted && !t.muted && c->gain > 0) {
            QString chain = QString("[%1:%2]asetpts=PTS-(%3)/"
                                    "TB,aresample=48000:async=1:first_pts=0,aformat=sample_fmts="
                                    "fltp:channel_layouts=stereo")
                                .arg(i)
                                .arg(a.audioStream)
                                .arg(seconds(a.startTime));
            if (c->fadeIn > 0)
                chain += QString(",afade=t=in:st=%1:d=%2")
                             .arg(seconds(c->envelopeIn), seconds(c->fadeIn));
            if (c->fadeOut > 0)
                chain += QString(",afade=t=out:st=%1:d=%2")
                             .arg(seconds(c->envelopeOut - c->fadeOut), seconds(c->fadeOut));
            QString label = QString("ac%1").arg(++an);
            chain += QString(",atrim=start=%1:end=%2,asetpts=PTS-(%1)/"
                             "TB,volume=%3,adelay=delays=%4S:all=1[%5]")
                         .arg(seconds(c->sourceIn), seconds(c->sourceOut),
                              QString::number(c->gain, 'f', 8))
                         .arg((c->start * 48000 + Second / 2) / Second)
                         .arg(label);
            f << chain;
            audio << "[" + label + "]";
        }
    }
    f << audio.join("") +
             QString("amix=inputs=%1:duration=longest:dropout_transition=0:normalize=0,alimiter="
                     "limit=0.98:level=0:latency=1,atrim=end=%2,asetpts=PTS-STARTPTS[aout]")
                 .arg(audio.size())
                 .arg(d);
    if (video)
        f << QString("[%1]trim=duration=%2,setpts=PTS-STARTPTS,format=yuv420p[vout]").arg(vbase, d);
    plan.filterGraph = f.join(";\n");
    return plan;
}
QStringList encodingArgs(const RenderProfile &p, const QString &encoder) {
    QStringList out;
    if (p.format == "mp4" || p.format == "mkv") {
        int bitrate = p.quality == "low" ? 4 : p.quality == "high" ? 16 : 8;
        if (std::min(p.width, p.height) <= 720)
            bitrate = std::max(2, bitrate / 2);
        out << "-map" << "[vout]" << "-map" << "[aout]" << "-c:v" << encoder << "-b:v"
            << QString::number(bitrate) + "M" << "-pix_fmt" << "yuv420p"
            << "-c:a" << "aac" << "-b:a" << "192k" << "-ar" << "48000" << "-ac" << "2";
        if (p.format == "mp4")
            out << "-movflags" << "+faststart";
        out << "-f" << (p.format == "mkv" ? "matroska" : "mp4");
    } else {
        out << "-map" << "[aout]" << "-vn" << "-c:a"
            << (p.format == "mp3" ? "libmp3lame" : "pcm_s16le") << "-ar" << "48000" << "-ac" << "2";
        if (p.format == "mp3")
            out << "-b:a" << "192k";
        out << "-f" << p.format;
    }
    return out;
}
} // namespace pms
