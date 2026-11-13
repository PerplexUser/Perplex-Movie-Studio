#pragma once
#include "commands/Editor.h"
#include "export/RenderJob.h"
#include "media/MediaTools.h"
#include "media/ProbeJob.h"
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
struct TestMedia {
    QTemporaryDir directory;
    int sequence = 0;
    QString path(const QString &n) const {
        return directory.filePath(n);
    }
    QByteArray run(QStringList args, const QString &tool = "ffmpeg") {
        QProcess p;
        p.start(pms::mediaTool(tool), QStringList{"-cpuflags", "0"} + args);
        if (!p.waitForStarted(10000) || !p.waitForFinished(60000) || p.exitCode())
            throw std::runtime_error(p.readAllStandardError().toStdString());
        return p.readAllStandardOutput();
    }
    pms::Asset probe(const QString &file) {
        pms::ProbeJob job;
        QSignalSpy good(&job, &pms::ProbeJob::succeeded), bad(&job, &pms::ProbeJob::failed);
        job.start(file);
        QElapsedTimer t;
        t.start();
        while (good.isEmpty() && bad.isEmpty() && t.elapsed() < 30000)
            QTest::qWait(10);
        if (good.isEmpty())
            throw std::runtime_error(bad.isEmpty() ? "Probe timeout"
                                                   : bad[0][0].toString().toStdString());
        return qvariant_cast<pms::Asset>(good[0][0]);
    }
    QString render(const pms::Project &p, int w = 320, int h = 180, QString out = {}) {
        pms::RenderJob job;
        QSignalSpy good(&job, &pms::RenderJob::succeeded), bad(&job, &pms::RenderJob::failed);
        pms::RenderProfile profile;
        profile.width = w;
        profile.height = h;
        if (out.isEmpty())
            out = path(QString("render-%1.mp4").arg(++sequence));
        job.start(p, profile, out, true);
        QElapsedTimer t;
        t.start();
        while (good.isEmpty() && bad.isEmpty() && t.elapsed() < 90000)
            QTest::qWait(10);
        if (good.isEmpty())
            throw std::runtime_error(
                bad.isEmpty() ? "Render timeout"
                              : (bad[0][0].toString() + "\n" + bad[0][1].toString()).toStdString());
        if (good[0][1].toString() != "mpeg4")
            throw std::runtime_error("Expected software MPEG4");
        return out;
    }
    QImage frame(const QString &file, double at, int w = 320, int h = 180) {
        auto b = run({"-v", "error", "-ss", QString::number(at, 'f', 6), "-i", file, "-frames:v",
                      "1", "-pix_fmt", "rgb24", "-f", "rawvideo", "-"});
        if (b.size() != w * h * 3)
            throw std::runtime_error("Frame size mismatch");
        return QImage(reinterpret_cast<const uchar *>(b.constData()), w, h, w * 3,
                      QImage::Format_RGB888)
            .copy();
    }
    QString color(const QString &n, const QString &c) {
        auto f = path(n + ".mp4");
        run({"-v", "error", "-y", "-f", "lavfi", "-i",
             QString("color=c=%1:s=320x180:r=30:d=2").arg(c), "-c:v", "mpeg4", "-pix_fmt",
             "yuv420p", f});
        return f;
    }
    pms::Editor edit(const QString &f, pms::Time d = 2 * pms::Second) {
        pms::Editor e;
        auto a = e.addAsset(probe(f));
        auto c = e.addClip(a, e.project().tracks[0].id, 0);
        e.trim(c, 0, d, 0);
        e.settings({320, 180, 30, 1});
        return e;
    }
};
