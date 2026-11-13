#include "export/RenderJob.h"
#include "media/MediaTools.h"
#include "render/VisualAssets.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>
#include <QtConcurrent/QtConcurrentRun>
#include <cerrno>
#include <cstring>
#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <cstdio>
#include <unistd.h>
#endif
namespace pms {
RenderJob::RenderJob(QObject *parent) : QObject(parent) {
    connect(&preparation_, &QFutureWatcher<PreparedRender>::finished, this, [this] {
        if (!active_)
            return;
        if (cancelled_) {
            active_ = false;
            cleanup();
            emit cancelled();
            return;
        }
        auto result = preparation_.result();
        if (result.domainError >= 0) {
            finishFailure(errorText(result.domainError));
            return;
        }
        if (!result.error.isEmpty()) {
            finishFailure(tr("Images or text could not be prepared."), result.error);
            return;
        }
        plan_ = result.plan;
        graph_ = std::make_unique<QTemporaryFile>(output_->filePath("graph-XXXXXX.txt"));
        auto bytes = plan_.filterGraph.toUtf8();
        if (!graph_->open() || graph_->write(bytes) != bytes.size()) {
            finishFailure(tr("Cannot create the temporary render instructions."));
            return;
        }
        graph_->close();
        phase_ = Detect;
        errors_.clear();
        progressBuffer_.clear();
        process_.start(mediaTool("ffmpeg"), {"-hide_banner", "-encoders"});
    });
    connect(&process_, &QProcess::readyReadStandardOutput, this, [this] {
        auto bytes = process_.readAllStandardOutput();
        if (phase_ != Render) {
            progressBuffer_ = (progressBuffer_ + bytes).right(1024 * 1024);
            return;
        }
        progressBuffer_ += bytes;
        while (progressBuffer_.contains('\n')) {
            auto pos = progressBuffer_.indexOf('\n');
            auto line = progressBuffer_.left(pos);
            progressBuffer_.remove(0, pos + 1);
            if (line.startsWith("out_time_us=")) {
                bool ok;
                auto t = line.mid(12).toLongLong(&ok);
                if (ok)
                    emit progress(std::clamp(double(t) / plan_.duration, 0.0, 0.999));
            }
        }
        if (progressBuffer_.size() > 32768)
            progressBuffer_.clear();
    });
    connect(&process_, &QProcess::readyReadStandardError, this,
            [this] { errors_ = (errors_ + process_.readAllStandardError()).right(65536); });
    connect(&process_, &QProcess::errorOccurred, this, [this](auto error) {
        if (error == QProcess::FailedToStart)
            finishFailure(tr("FFmpeg could not be started. Check the media tools installation."),
                          process_.errorString());
    });
    connect(
        &process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        [this](int code, auto status) {
            if (!active_)
                return;
            if (cancelled_) {
                active_ = false;
                cleanup();
                emit cancelled();
                return;
            }
            if (phase_ == Detect) {
                if (code != 0) {
                    finishFailure(tr("The installed FFmpeg could not report its encoders."),
                                  QString::fromUtf8(errors_));
                    return;
                }
                bool video = profile_.format == "mp4" || profile_.format == "mkv";
                QString required = video                      ? "mpeg4"
                                   : profile_.format == "mp3" ? "libmp3lame"
                                                              : "pcm_s16le";
                if (!progressBuffer_.contains(required.toUtf8()) ||
                    (video && !progressBuffer_.contains(" aac "))) {
                    finishFailure(tr("The installed FFmpeg does not provide the encoders required "
                                     "for this format."));
                    return;
                }
                beginRender();
                return;
            }
            if (code != 0 || status != QProcess::NormalExit) {
                finishFailure(tr("Export failed. Check free disk space, source files, and encoder "
                                 "support. The previous destination has been preserved."),
                              QString::fromUtf8(errors_));
                return;
            }
            if (!commitOutput()) {
                finishFailure(tr("The completed export could not replace the destination. Check "
                                 "write permissions and close other programs using the file."),
                              commitError_);
                return;
            }
            active_ = false;
            cleanup();
            emit progress(1);
            emit succeeded(destination_, encoder_);
        });
}
RenderJob::~RenderJob() {
    preparation_.waitForFinished();
    process_.kill();
    process_.waitForFinished(1500);
    cleanup();
}
void RenderJob::start(const Project &p, const RenderProfile &profile, const QString &destination,
                      bool overwrite) {
    if (active_)
        return;
    active_ = true;
    cancelled_ = false;
    profile_ = profile;
    destination_ = QFileInfo(destination).absoluteFilePath();
    overwrite_ = overwrite;
    encoder_ = profile.format == "mp3"   ? "libmp3lame"
               : profile.format == "wav" ? "pcm_s16le"
                                         : "mpeg4";
    auto deferFail = [this](const QString &message) {
        QTimer::singleShot(0, this, [this, message] { finishFailure(message); });
    };
    if (mediaTool("ffmpeg").isEmpty()) {
        deferFail(tr("FFmpeg was not found. Install the media tools first."));
        return;
    }
    for (const auto &a : p.assets) {
        if (a.kind == AssetKind::Title)
            continue;
        QFileInfo f(QString::fromStdString(a.path));
        auto target = QFileInfo(destination_);
        if (f.absoluteFilePath() == destination_ ||
            (!target.canonicalFilePath().isEmpty() &&
             f.canonicalFilePath() == target.canonicalFilePath())) {
            deferFail(tr("Choose a destination different from every source media file."));
            return;
        }
    }
    if (QFileInfo::exists(destination_) && !overwrite_) {
        deferFail(
            tr("The destination already exists. Confirm overwriting or choose another name."));
        return;
    }
    try {
        p.validate();
    } catch (const DomainError &e) {
        deferFail(errorText(int(e.code)));
        return;
    }
    output_ = std::make_unique<QTemporaryDir>(QFileInfo(destination_).absolutePath() +
                                              "/.pms-export-XXXXXX");
    if (!output_->isValid()) {
        deferFail(tr("Cannot write in the destination folder."));
        return;
    }
    outputPath_ = output_->filePath("render." + profile.format);
    phase_ = Prepare;
    auto directory = output_->path();
    preparation_.setFuture(QtConcurrent::run([p, profile, directory] {
        PreparedRender result;
        try {
            result.plan = compose(p, profile, prepareVisualAssets(p, profile, directory));
        } catch (const DomainError &e) {
            result.domainError = int(e.code);
        } catch (const std::exception &e) {
            result.error = QString::fromUtf8(e.what());
        }
        return result;
    }));
}

void RenderJob::beginRender() {
    phase_ = Render;
    errors_.clear();
    progressBuffer_.clear();
    QStringList args = {"-cpuflags", "0",       "-hide_banner",
                        "-loglevel", "error",   "-nostdin",
                        "-y",        "-copyts", "-filter_complex_threads",
                        "2"};
    args += plan_.inputArgs;
    args << "-filter_complex_script" << graph_->fileName();
    args += encodingArgs(profile_, encoder_);
    args << "-t" << seconds(plan_.duration) << "-threads" << "2" << "-progress" << "pipe:1"
         << "-stats_period" << "0.1" << outputPath_;
    process_.start(mediaTool("ffmpeg"), args);
}
bool RenderJob::commitOutput() {
    commitError_.clear();
    if (!output_ || QFileInfo(outputPath_).size() == 0) {
        commitError_ = tr("The rendered temporary file is missing or empty.");
        return false;
    }
#ifdef Q_OS_WIN
    auto src = QDir::toNativeSeparators(outputPath_).toStdWString(),
         dst = QDir::toNativeSeparators(destination_).toStdWString();
    if (MoveFileExW(src.c_str(), dst.c_str(),
                    MOVEFILE_WRITE_THROUGH | (overwrite_ ? MOVEFILE_REPLACE_EXISTING : 0)))
        return true;
    DWORD code = GetLastError();
    wchar_t message[2048] = {};
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code, 0,
                   message, 2048, nullptr);
    commitError_ =
        tr("Windows error %1: %2").arg(code).arg(QString::fromWCharArray(message).trimmed());
    return false;
#else
    auto src = QFile::encodeName(outputPath_), dst = QFile::encodeName(destination_);
    if (overwrite_) {
        if (::rename(src.constData(), dst.constData()) == 0)
            return true;
    } else if (::link(src.constData(), dst.constData()) == 0) {
        ::unlink(src.constData());
        return true;
    }
    commitError_ = QString::fromLocal8Bit(std::strerror(errno));
    return false;
#endif
}
void RenderJob::cancel() {
    if (!active_)
        return;
    cancelled_ = true;
    if (phase_ == Prepare && preparation_.isRunning())
        return;
    if (process_.state() == QProcess::NotRunning) {
        active_ = false;
        cleanup();
        emit cancelled();
        return;
    }
    process_.kill();
}
void RenderJob::finishFailure(const QString &message, const QString &detail) {
    if (!active_)
        return;
    active_ = false;
    process_.kill();
    cleanup();
    auto dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dir);
    QFile previous(dir + "/render-errors.log");
    QByteArray entries;
    if (previous.open(QIODevice::ReadOnly)) {
        if (previous.size() > 512 * 1024)
            previous.seek(previous.size() - 512 * 1024);
        entries = previous.readAll();
        previous.close();
    }
    entries += QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toUtf8() + " " +
               message.toUtf8() + "\n" + detail.toUtf8().right(65536) + "\n";
    QSaveFile log(dir + "/render-errors.log");
    if (log.open(QIODevice::WriteOnly)) {
        log.write(entries);
        log.commit();
    }
    emit failed(message, detail);
}
void RenderJob::cleanup() {
    graph_.reset();
    output_.reset();
    outputPath_.clear();
}
} // namespace pms
