#include "commands/Editor.h"
#include "export/RenderJob.h"
#include "media/MediaTools.h"
#include "media/ProbeJob.h"
#include "persistence/ProjectStore.h"
#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>
#include <cstring>

using namespace pms;
class ServiceTests : public QObject {
    Q_OBJECT
    QTemporaryDir dir_;
    QString red_, blue_, tone_;
    QByteArray run(const QStringList &args, const QString &tool = "ffmpeg") {
        QProcess p;
        p.start(mediaTool(tool), QStringList{"-cpuflags", "0"} + args);
        if (!p.waitForStarted(10000) || !p.waitForFinished(60000) || p.exitCode() != 0)
            throw std::runtime_error(("Process failed: " + p.errorString() + "\n" +
                                      QString::fromUtf8(p.readAllStandardError()))
                                         .toStdString());
        return p.readAllStandardOutput();
    }
    Asset probe(const QString &path) {
        ProbeJob job;
        QSignalSpy good(&job, &ProbeJob::succeeded), bad(&job, &ProbeJob::failed);
        job.start(path);
        QElapsedTimer timer;
        timer.start();
        while (good.isEmpty() && bad.isEmpty() && timer.elapsed() < 35000)
            QTest::qWait(10);
        if (good.isEmpty())
            throw std::runtime_error(bad.isEmpty() ? "probe timed out"
                                                   : bad.first()[0].toString().toStdString());
        return qvariant_cast<Asset>(good.first()[0]);
    }
    Project project() {
        Editor e;
        auto a = e.addAsset(probe(red_));
        e.addClip(a, e.project().tracks[0].id, 0);
        return e.project();
    }
    QString render(const Project &p, const QString &format, const QString &name = "output",
                   int width = 320, int height = 180) {
        RenderJob job;
        QSignalSpy good(&job, &RenderJob::succeeded), bad(&job, &RenderJob::failed);
        RenderProfile profile;
        profile.format = format;
        profile.width = width;
        profile.height = height;
        auto path = dir_.path() + "/" + name + "." + format;
        job.start(p, profile, path, true);
        QElapsedTimer timer;
        timer.start();
        while (good.isEmpty() && bad.isEmpty() && timer.elapsed() < 90000)
            QTest::qWait(10);
        if (good.isEmpty())
            throw std::runtime_error(
                bad.isEmpty()
                    ? "render timed out"
                    : (bad.first()[0].toString() + "\n" + bad.first()[1].toString()).toStdString());
        return path;
    }
    QJsonObject metadata(const QString &path) {
        return QJsonDocument::fromJson(
                   run({"-v", "error", "-show_streams", "-show_format", "-of", "json", path},
                       "ffprobe"))
            .object();
    }
    QByteArray pcm(const QString &path) {
        return run(
            {"-v", "error", "-i", path, "-vn", "-ar", "48000", "-ac", "2", "-f", "s16le", "-"});
    }
    int peak(const QByteArray &bytes) {
        int value = 0;
        for (qsizetype i = 0; i + 1 < bytes.size(); i += 2) {
            qint16 s;
            std::memcpy(&s, bytes.constData() + i, 2);
            value = std::max(value, std::abs(int(s)));
        }
        return value;
    }
    QByteArray pixels(const QString &path, const QString &at) {
        return run({"-v", "error", "-ss", at, "-i", path, "-frames:v", "1", "-vf", "scale=16:16",
                    "-pix_fmt", "rgb24", "-f", "rawvideo", "-"});
    }
  private slots:
    void initTestCase() {
        if (qEnvironmentVariableIsSet("PMS_KEEP_TEST_OUTPUT")) {
            dir_.setAutoRemove(false);
            qInfo() << "Fixtures:" << dir_.path();
        }
        qRegisterMetaType<pms::Asset>();
        QVERIFY(dir_.isValid());
        QVERIFY(!mediaTool("ffmpeg").isEmpty());
        QVERIFY(!mediaTool("ffprobe").isEmpty());
        red_ = dir_.path() + "/Röt € ; clip.mp4";
        blue_ = dir_.path() + "/blue.mkv";
        tone_ = dir_.path() + "/tone.wav";
        run({"-v", "error", "-y", "-f", "lavfi", "-i", "color=c=red:s=320x180:r=30:d=2", "-f",
             "lavfi", "-i", "sine=frequency=440:sample_rate=48000:duration=2", "-c:v", "mpeg4",
             "-c:a", "aac", "-shortest", red_});
        run({"-v", "error", "-y", "-f", "lavfi", "-i", "color=c=blue:s=320x180:r=30:d=2", "-c:v",
             "mpeg4", blue_});
        run({"-v", "error", "-y", "-f", "lavfi", "-i",
             "sine=frequency=880:sample_rate=48000:duration=2", "-c:a", "pcm_s16le", tone_});
    }
    void probeMetadataAndInvalid() {
        auto a = probe(red_);
        QCOMPARE(a.width, 320);
        QCOMPARE(a.height, 180);
        QCOMPARE(a.videoCodec, std::string("mpeg4"));
        QVERIFY(a.audioStream >= 0);
        QVERIFY(std::abs(a.duration - 2 * Second) < 100000);
        QString renamed = dir_.path() + "/actually-video.xyz";
        QVERIFY(QFile::copy(red_, renamed));
        QCOMPARE(probe(renamed).videoCodec, std::string("mpeg4"));
        QString bad = dir_.path() + "/invalid.mp4";
        QFile f(bad);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("not a video");
        f.close();
        ProbeJob job;
        QSignalSpy failed(&job, &ProbeJob::failed);
        job.start(bad);
        QVERIFY(failed.wait(10000));
    }
    void formatsImport() {
        struct Format {
            QString ext, v, a;
        };
        const QList<Format> formats = {{"avi", "mpeg4", "libmp3lame"},
                                       {"mpg", "mpeg1video", "mp2"},
                                       {"mpeg", "mpeg2video", "mp2"},
                                       {"mp3", "", "libmp3lame"},
                                       {"wav", "", "pcm_s24le"}};
        for (const auto &f : formats) {
            auto path = dir_.path() + "/matrix." + f.ext;
            QStringList args = {"-v", "error", "-y", "-i", red_};
            if (f.v.isEmpty())
                args << "-vn";
            else
                args << "-r" << "25" << "-c:v" << f.v;
            args << "-c:a" << f.a << path;
            run(args);
            auto a = probe(path);
            QVERIFY(a.audioStream >= 0);
            if (!f.v.isEmpty())
                QVERIFY(a.videoStream >= 0);
        }
        const auto fixtures = QCoreApplication::applicationDirPath() + "/fixtures/";
        auto h264 = probe(fixtures + "h264-aac.mp4");
        QCOMPARE(h264.videoCodec, std::string("h264"));
        QCOMPARE(h264.audioCodec, std::string("aac"));
        auto vp9 = probe(fixtures + "vp9-opus.mkv");
        QCOMPARE(vp9.videoCodec, std::string("vp9"));
        QCOMPARE(vp9.audioCodec, std::string("opus"));
        auto h264mkv = dir_.path() + "/h264.mkv";
        run({"-v", "error", "-y", "-i", fixtures + "h264-aac.mp4", "-c", "copy", h264mkv});
        QCOMPARE(probe(h264mkv).videoCodec, std::string("h264"));
        auto vbr = dir_.path() + "/vbr.mp3";
        run({"-v", "error", "-y", "-i", tone_, "-c:a", "libmp3lame", "-q:a", "2", vbr});
        QCOMPARE(probe(vbr).audioCodec, std::string("mp3"));
    }
    void atomicPersistenceAndRelink() {
        auto p = project();
        auto path = dir_.path() + "/project.pmsproj";
        ProjectStore::save(p, path);
        QCOMPARE(ProjectStore::load(path), p);
        auto object = ProjectStore::encode(p, path);
        QVERIFY(!QDir::isAbsolutePath(object["assets"].toArray()[0].toObject()["path"].toString()));
        object["schemaVersion"] = 3;
        QVERIFY_EXCEPTION_THROWN(ProjectStore::decode(object, path), DomainError);
        object = ProjectStore::encode(p, path);
        auto clips = object["clips"].toArray();
        auto c = clips[0].toObject();
        c["sourceOut"] = "2";
        clips[0] = c;
        object["clips"] = clips;
        QVERIFY_EXCEPTION_THROWN(ProjectStore::decode(object, path), DomainError);
        QFile source(red_);
        QVERIFY(source.open(QIODevice::ReadOnly));
        auto hash = QCryptographicHash::hash(source.readAll(), QCryptographicHash::Sha256);
        source.close();
        Editor e(p);
        auto id = p.clips.front().id;
        e.split(id, Second);
        e.undo();
        e.redo();
        e.markSaved();
        ProjectStore::save(e.project(), path);
        QCOMPARE(ProjectStore::load(path), e.project());
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(QCryptographicHash::hash(source.readAll(), QCryptographicHash::Sha256), hash);
        source.close();
        auto moved = red_ + ".moved";
        QVERIFY(QFile::rename(red_, moved));
        auto missing = ProjectStore::load(path);
        QCOMPARE(ProjectStore::missing(missing).size(), 1);
        Editor relink(missing);
        relink.relink(p.assets[0].id, probe(moved));
        QVERIFY(ProjectStore::missing(relink.project()).isEmpty());
        QVERIFY(QFile::rename(moved, red_));
        QFile original(path);
        QVERIFY(original.open(QIODevice::ReadOnly));
        auto saved = original.readAll();
        original.close();
        auto invalid = p;
        invalid.schemaVersion = 8;
        QVERIFY_EXCEPTION_THROWN(ProjectStore::save(invalid, path), DomainError);
        QVERIFY(original.open(QIODevice::ReadOnly));
        QCOMPARE(original.readAll(), saved);
    }
    void exportAllFourFormats() {
        auto p = project();
        for (const auto &format : {"mp4", "mkv", "mp3", "wav"}) {
            auto path = render(p, format, QString("format-") + format);
            auto meta = metadata(path);
            auto streams = meta["streams"].toArray();
            int video = 0, audio = 0;
            for (const auto s : streams) {
                auto o = s.toObject();
                if (o["codec_type"] == "video") {
                    ++video;
                    QCOMPARE(o["width"].toInt(), 320);
                    QCOMPARE(o["height"].toInt(), 180);
                    QCOMPARE(o["r_frame_rate"].toString(), QString("30/1"));
                }
                if (o["codec_type"] == "audio")
                    ++audio;
            }
            QCOMPARE(audio, 1);
            QCOMPARE(video, QString(format) == "mp4" || QString(format) == "mkv" ? 1 : 0);
            QVERIFY(peak(pcm(path)) > 500);
            auto duration = meta["format"].toObject()["duration"].toString().toDouble();
            QVERIFY(std::abs(duration - 2.0) < 0.08);
        }
    }
    void silenceAndAudioOnly() {
        auto p = project();
        p.clips[0].muted = true;
        for (const auto &format : {"mp4", "mkv"}) {
            auto path = render(p, format, "silent");
            QCOMPARE(metadata(path)["streams"].toArray().size(), 2);
            QVERIFY(peak(pcm(path)) <= 1);
        }
        Editor e;
        auto a = e.addAsset(probe(tone_));
        e.addClip(a, e.project().tracks[2].id, 0);
        auto path = render(e.project(), "mp4", "audio-only");
        auto px = pixels(path, "0.5");
        QVERIFY(!px.isEmpty());
        for (auto c : px)
            QVERIFY2(static_cast<unsigned char>(c) < 8, px.toHex().constData());
        QVERIFY(peak(pcm(path)) > 500);
    }
    void layeringGapsAndSourceRanges() {
        Editor e;
        auto a = e.addAsset(probe(red_)), b = e.addAsset(probe(blue_));
        auto v1 = e.project().tracks[0].id, v2 = e.project().tracks[1].id;
        auto id = e.addClip(a, v1, 0);
        e.trim(id, 0, Second, 0);
        id = e.addClip(b, v2, Second / 2);
        e.trim(id, 0, Second, Second / 2);
        id = e.addClip(a, v1, 2 * Second);
        e.trim(id, Second, 2 * Second, 2 * Second);
        auto path = render(e.project(), "mp4", "layers");
        auto red = pixels(path, "0.2"), blue = pixels(path, "0.8"), gap = pixels(path, "1.8");
        QVERIFY(red.size() >= 3 && blue.size() >= 3 && gap.size() >= 3);
        QVERIFY2(static_cast<unsigned char>(red[0]) > 180, red.toHex().constData());
        QVERIFY2(static_cast<unsigned char>(blue[2]) > 180, blue.toHex().constData());
        for (auto c : gap)
            QVERIFY2(static_cast<unsigned char>(c) < 8, gap.toHex().constData());
    }
    void fadeSplitMaintainsAudio() {
        Editor e;
        auto a = e.addAsset(probe(tone_));
        auto id = e.addClip(a, e.project().tracks[2].id, 0);
        e.audio(id, 0.6, false, Second, Second);
        auto original = pcm(render(e.project(), "wav", "unsplit"));
        e.split(id, Second / 2);
        auto split = pcm(render(e.project(), "wav", "split"));
        QCOMPARE(original.size(), split.size());
        int maxError = 0;
        for (qsizetype i = 0; i + 1 < original.size(); i += 2) {
            qint16 a, b;
            std::memcpy(&a, original.constData() + i, 2);
            std::memcpy(&b, split.constData() + i, 2);
            maxError = std::max(maxError, std::abs(int(a) - int(b)));
        }
        QVERIFY2(maxError <= 2, qPrintable(QString::number(maxError)));
    }
    void nonzeroTimestampsAndVfr() {
        auto shifted = dir_.path() + "/shifted.mp4";
        run({"-v", "error", "-y", "-i", red_, "-c", "copy", "-output_ts_offset", "5", shifted});
        auto a = probe(shifted);
        QVERIFY(a.startTime > 4 * Second);
        QVERIFY(a.duration < 3 * Second);
        Editor e;
        auto id = e.addAsset(a);
        e.addClip(id, e.project().tracks[0].id, 0);
        auto path = render(e.project(), "mp4", "shifted-normalized");
        auto rgb = pixels(path, "0.3");
        QVERIFY(static_cast<unsigned char>(rgb[0]) > 180);
        QVERIFY(peak(pcm(path)) > 500);
        auto streams = metadata(path)["streams"].toArray();
        double av = -1, vv = -1;
        for (const auto s : streams) {
            auto o = s.toObject();
            if (o["codec_type"] == "audio")
                av = o["start_time"].toString().toDouble();
            if (o["codec_type"] == "video")
                vv = o["start_time"].toString().toDouble();
        }
        QVERIFY(std::abs(av - vv) <= 0.04);
        auto vfr = dir_.path() + "/vfr.mkv";
        run({"-v", "error", "-y", "-f", "lavfi", "-i", "testsrc2=size=320x180:rate=30:duration=2",
             "-vf", "select='if(lt(t,1),1,not(mod(n,2)))'", "-fps_mode", "vfr", "-c:v", "mpeg4",
             vfr});
        Editor v;
        id = v.addAsset(probe(vfr));
        v.addClip(id, v.project().tracks[0].id, 0);
        auto output = render(v.project(), "mkv", "vfr-cfr");
        auto meta = metadata(output);
        QCOMPARE(meta["streams"].toArray()[0].toObject()["r_frame_rate"].toString(),
                 QString("30/1"));
        QVERIFY(std::abs(meta["format"].toObject()["duration"].toString().toDouble() -
                         double(v.project().duration()) / Second) < 0.07);
    }
    void trackMuteAnd1080Profile() {
        Editor e(project());
        auto a = e.addAsset(probe(tone_));
        e.addClip(a, e.project().tracks[2].id, 0);
        e.addClip(a, e.project().tracks[3].id, 0);
        e.trackState(e.project().tracks[0].id, true, false);
        e.trackState(e.project().tracks[2].id, true, false);
        e.trackState(e.project().tracks[3].id, true, false);
        QVERIFY(peak(pcm(render(e.project(), "wav", "all-tracks-muted"))) <= 1);
        auto c = e.project().clips[0];
        e.trim(c.id, 0, 200000, 0);
        e.erase(e.project().clips.back().id);
        e.erase(e.project().clips.back().id);
        auto output = render(e.project(), "mp4", "1080-profile", 1920, 1080);
        auto stream = metadata(output)["streams"].toArray()[0].toObject();
        QCOMPARE(stream["width"].toInt(), 1920);
        QCOMPARE(stream["height"].toInt(), 1080);
    }
    void cancelPreservesDestination() {
        auto p = project();
        auto path = dir_.path() + "/preserved.mp4";
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("existing bytes");
        f.close();
        RenderJob job;
        QSignalSpy cancelled(&job, &RenderJob::cancelled), success(&job, &RenderJob::succeeded);
        RenderProfile profile;
        profile.width = 1920;
        profile.height = 1080;
        job.start(p, profile, path, true);
        QTimer::singleShot(60, &job, &RenderJob::cancel);
        QVERIFY(cancelled.wait(10000));
        QVERIFY(success.isEmpty());
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), QByteArray("existing bytes"));
        QVERIFY(
            QDir(dir_.path()).entryList({".pms-export-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
    void badDestinationAndSourceProtection() {
        auto p = project();
        for (const auto &path : {red_, dir_.path() + "/missing/fail.mp4"}) {
            RenderJob job;
            QSignalSpy failure(&job, &RenderJob::failed);
            job.start(p, RenderProfile{}, path, true);
            QVERIFY(failure.wait(5000));
        }
        RenderJob empty;
        QSignalSpy failure(&empty, &RenderJob::failed);
        empty.start(makeProject(), RenderProfile{}, dir_.path() + "/empty.mp4", false);
        QVERIFY(failure.wait(5000));
    }
};
QTEST_MAIN(ServiceTests)
#include "service_tests.moc"
