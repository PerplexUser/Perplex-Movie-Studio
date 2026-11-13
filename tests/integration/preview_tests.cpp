#include "ui/PreviewWidget.h"
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QSignalSpy>
#include <QVideoFrame>
#include <QVideoSink>
#include <QtTest>

class PreviewTests : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() {
        qputenv("QT_MEDIA_BACKEND", "ffmpeg");
        qputenv("QT_FFMPEG_DECODING_HW_DEVICE_TYPES", ",");
        qputenv("QT_DISABLE_HW_TEXTURES_CONVERSION", "1");
    }
    void playsAndSeeksLocalMedia() {
        pms::PreviewWidget monitor;
        QVERIFY(pms::configureSoftwarePlayback());
        monitor.resize(640, 360);
        monitor.show();
        QMediaPlayer player;
        QAudioOutput audio;
        audio.setVolume(0);
        player.setAudioOutput(&audio);
        player.setVideoSink(monitor.videoSink());
        QSignalSpy frames(monitor.videoSink(), &QVideoSink::videoFrameChanged);
        const auto file = QCoreApplication::applicationDirPath() + "/fixtures/h264-aac.mp4";
        QVERIFY(QFileInfo::exists(file));
        player.setSource(QUrl::fromLocalFile(file));
        player.play();
        QTRY_VERIFY_WITH_TIMEOUT(frames.count() >= 5, 8000);
        QVERIFY2(player.error() == QMediaPlayer::NoError, qPrintable(player.errorString()));
        player.pause();
        frames.clear();
        player.setPosition(500);
        QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty(), 5000);
        auto frame = monitor.videoSink()->videoFrame();
        QVERIFY(frame.isValid());
        const auto image = frame.toImage();
        QVERIFY(!image.isNull());
        QCOMPARE(image.size(), QSize(160, 90));
        player.stop();
        player.setSource({});
    }
};
QTEST_MAIN(PreviewTests)
#include "preview_tests.moc"
