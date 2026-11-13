#include "media/ProbeJob.h"
#include "media/MediaTools.h"
#include <QFileInfo>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtConcurrent/QtConcurrentRun>
#include <cmath>
namespace pms {
ProbeJob::ProbeJob(QObject *parent) : QObject(parent) {
    connect(&imageProbe_, &QFutureWatcher<Asset>::finished, this, [this] {
        if (finished_)
            return;
        auto a = imageProbe_.result();
        if (a.duration <= 0) {
            fail(tr("This image cannot be read or exceeds the supported size."));
            return;
        }
        finished_ = true;
        timeout_.stop();
        emit succeeded(a);
    });
    timeout_.setSingleShot(true);
    timeout_.setInterval(30000);
    connect(&timeout_, &QTimer::timeout, this,
            [this] { fail(tr("Reading this media took too long.")); });
    connect(&process_, &QProcess::readyReadStandardOutput, this, [this] {
        data_ += process_.readAllStandardOutput();
        if (data_.size() > 4 * 1024 * 1024)
            fail(tr("The media metadata exceeds the supported limit."));
    });
    connect(&process_, &QProcess::readyReadStandardError, this,
            [this] { errors_ = (errors_ + process_.readAllStandardError()).right(32768); });
    connect(&process_, &QProcess::errorOccurred, this, [this](auto e) {
        if (e == QProcess::FailedToStart)
            fail(tr("FFmpeg or ffprobe could not be started. Check the media tools installation."),
                 process_.errorString());
    });
    connect(&process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int code, auto status) {
                if (finished_)
                    return;
                if (code != 0 || status != QProcess::NormalExit) {
                    fail(tr("This file could not be decoded. It may be damaged, protected, or use "
                            "an unsupported codec."),
                         QString::fromUtf8(errors_));
                    return;
                }
                if (decoding_) {
                    finished_ = true;
                    timeout_.stop();
                    emit succeeded(asset_);
                    return;
                }
                try {
                    asset_ = parse(data_, path_);
                } catch (const std::exception &) {
                    fail(tr("This file has no supported audio or video stream, or its duration is "
                            "invalid."));
                    return;
                }
                decoding_ = true;
                data_.clear();
                errors_.clear();
                QStringList args = {"-v", "error", "-cpuflags", "0", "-nostdin", "-threads", "2"};
                args += localInput(path_);
                if (asset_.videoStream >= 0)
                    args << "-map" << QString("0:%1").arg(asset_.videoStream);
                if (asset_.audioStream >= 0)
                    args << "-map" << QString("0:%1").arg(asset_.audioStream);
                args << "-t" << "0.05" << "-f" << "null" << "-";
                process_.start(mediaTool("ffmpeg"), args);
            });
}
ProbeJob::~ProbeJob() {
    imageProbe_.waitForFinished();
    process_.kill();
    process_.waitForFinished(1000);
}
void ProbeJob::start(const QString &path) {
    path_ = QFileInfo(path).absoluteFilePath();
    if (!QFileInfo(path_).isFile()) {
        QTimer::singleShot(0, this,
                           [this] { fail(tr("The source file is missing or cannot be read.")); });
        return;
    }
    auto imageFormat = QImageReader::imageFormat(path_);
    if (imageFormat == "png" || imageFormat == "jpeg" || imageFormat == "jpg") {
        timeout_.start();
        auto file = path_;
        imageProbe_.setFuture(QtConcurrent::run([file] {
            Asset a;
            QImageReader reader(file);
            reader.setAutoTransform(true);
            auto sz = reader.size();
            if (!sz.isValid() || sz.width() > 8192 || sz.height() > 8192 ||
                qint64(sz.width()) * sz.height() > 32 * 1024 * 1024 ||
                QFileInfo(file).size() > 128 * 1024 * 1024)
                return a;
            auto image = reader.read();
            if (image.isNull())
                return a;
            a.id = newId();
            a.kind = AssetKind::Image;
            a.path = file.toStdString();
            a.name = QFileInfo(file).fileName().toStdString();
            a.container = "image";
            a.videoCodec = reader.format().toStdString();
            a.videoStream = 0;
            a.duration = MaxTime;
            a.width = image.width();
            a.height = image.height();
            return a;
        }));
        return;
    }
    if (mediaTool("ffprobe").isEmpty() || mediaTool("ffmpeg").isEmpty()) {
        QTimer::singleShot(0, this, [this] {
            fail(tr("FFmpeg and ffprobe are required. Place them in the media folder beside the "
                    "application."));
        });
        return;
    }
    QStringList args = {"-v",           "error",         "-cpuflags", "0",
                        "-show_format", "-show_streams", "-of",       "json"};
    args += localInput(path_);
    timeout_.start();
    process_.start(mediaTool("ffprobe"), args);
}
void ProbeJob::fail(const QString &message, const QString &detail) {
    if (finished_)
        return;
    finished_ = true;
    timeout_.stop();
    process_.kill();
    emit failed(message, detail);
}
void ProbeJob::cancel() {
    fail(tr("Import cancelled."));
}
Asset ProbeJob::parse(const QByteArray &data, const QString &path) {
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject())
        throw DomainError(Error::InvalidProject);
    Asset a;
    a.id = newId();
    a.path = QFileInfo(path).absoluteFilePath().toStdString();
    a.name = QFileInfo(path).fileName().toStdString();
    const auto obj = doc.object(), fmt = obj["format"].toObject();
    a.container = fmt["format_name"].toString().toStdString();
    // A successful probe is not an extension check. Only local, finite media containers are
    // accepted.
    QStringList containers = QString::fromStdString(a.container).split(',');
    const QStringList allowed = {"avi", "mpeg", "mpegvideo", "mov",  "mp4", "m4a", "3gp",
                                 "3g2", "mj2",  "matroska",  "webm", "mp3", "wav"};
    bool supported = false;
    for (const auto &c : containers)
        if (allowed.contains(c))
            supported = true;
    if (!supported)
        throw DomainError(Error::InvalidProject);
    double duration = fmt["duration"].toString().toDouble();
    double origin = fmt["start_time"].toString().toDouble();
    if (!std::isfinite(origin) || std::abs(origin) > double(MaxTime) / Second)
        throw DomainError(Error::InvalidProject);
    a.startTime = std::llround(origin * Second);
    for (const auto v : obj["streams"].toArray()) {
        auto s = v.toObject();
        auto type = s["codec_type"].toString();
        int index = s["index"].toInt(-1);
        if (type == "video" && a.videoStream < 0 &&
            !s["disposition"].toObject()["attached_pic"].toInt()) {
            a.videoStream = index;
            a.width = s["width"].toInt();
            a.height = s["height"].toInt();
            a.videoCodec = s["codec_name"].toString().toStdString();
            auto fps = s["avg_frame_rate"].toString().split('/');
            if (fps.size() == 2) {
                a.fpsNum = fps[0].toInt();
                a.fpsDen = std::max(1, fps[1].toInt());
            }
            for (const auto sd : s["side_data_list"].toArray())
                if (sd.toObject().contains("rotation"))
                    a.rotation = sd.toObject()["rotation"].toInt();
        }
        if (type == "audio" && a.audioStream < 0) {
            a.audioStream = index;
            a.audioCodec = s["codec_name"].toString().toStdString();
            a.sampleRate = s["sample_rate"].toString().toInt();
            a.channels = s["channels"].toInt();
        }
        if (duration <= 0)
            duration = std::max(duration, s["duration"].toString().toDouble());
    }
    if (!std::isfinite(duration) || duration <= 0 || duration > double(MaxTime) / Second)
        throw DomainError(Error::InvalidRange);
    a.duration = std::llround(duration * Second);
    auto p = makeProject();
    p.assets.push_back(a);
    p.validate();
    return a;
}
} // namespace pms
