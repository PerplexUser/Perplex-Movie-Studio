#include "TestMedia.h"
#include "ui/MainWindow.h"
#include "ui/PreviewWidget.h"
#include "ui/Theme.h"
#include <QAudioOutput>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QMediaPlayer>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStyleFactory>
using namespace pms;
class WindowTests : public QObject {
    Q_OBJECT
    TestMedia m;
    template <class T> T *widget(MainWindow &w, const char *n) {
        auto *p = w.findChild<T *>(n);
        if (!p)
            throw std::runtime_error(n);
        return p;
    }
    void number(MainWindow &w, const char *n, double v) {
        auto *p = widget<QDoubleSpinBox>(w, n);
        p->setValue(v);
        QMetaObject::invokeMethod(p, "editingFinished");
    }
  private slots:
    void initTestCase() {
        qRegisterMetaType<Asset>();
        qApp->setStyle(QStyleFactory::create("Fusion"));
        qApp->setStyleSheet(pms::Theme);
        QCoreApplication::setOrganizationName("pms-tests");
        QCoreApplication::setApplicationName("workflow");
        QVERIFY(configureSoftwarePlayback());
    }
    void about() {
        MainWindow w(true);
        w.setLanguage("de-DE", false);
        QCOMPARE(w.aboutText(),
                 QString::fromUtf8(
                     "Perplex Movie Studio\nVersion 0.1.1-dev\n\n© by www.perplex.click\nMichael "
                     "Lappenbusch\n\nEigener Anwendungscode: MIT-Lizenz.\nQt: © The Qt Company "
                     "Ltd. und Mitwirkende, LGPL v3.\nFFmpeg: LGPL v2.1 oder neuer im "
                     "mitgelieferten Build.\nLAME: LGPL v2 oder neuer.\n\nSiehe LICENSE und "
                     "THIRD_PARTY_NOTICES.md neben der Anwendung.\nEntwicklungsversion: "
                     "Windows-Abnahme und Leistungstests stehen noch aus."));
    }
    void workflow() {
        MainWindow w(true);
        w.resize(1440, 960);
        w.show();
        auto &s = w.session();
        w.importFiles({m.color("red-ui", "red")});
        QTRY_COMPARE_WITH_TIMEOUT(s.project().assets.size(), size_t(1), 10000);
        s.edit([&](Editor &e) {
            s.selected = e.addClip(e.project().assets[0].id, e.project().tracks[0].id, 0);
        });
        auto *play = widget<QPushButton>(w, "playButton"),
             *next = widget<QPushButton>(w, "nextFrameButton");
        auto *preview = widget<PreviewWidget>(w, "videoPreview");
        auto *player = w.findChild<QMediaPlayer *>();
        QVERIFY(player);
        w.findChild<QAudioOutput *>()->setVolume(0);
        QTest::mouseClick(next, Qt::LeftButton);
        QTRY_VERIFY_WITH_TIMEOUT(preview->hasFrame(), 30000);
        QCOMPARE(player->playbackState(), QMediaPlayer::PausedState);
        auto pixel = preview->frameImage().pixelColor(640, 360);
        QVERIFY(pixel.red() > 220 && pixel.green() < 25 && pixel.blue() < 25);
        QTest::mouseClick(play, Qt::LeftButton);
        QTRY_COMPARE(player->playbackState(), QMediaPlayer::PlayingState);
        QTRY_VERIFY(s.playhead > 200000);
        QTest::mouseClick(play, Qt::LeftButton);
        QCOMPARE(player->playbackState(), QMediaPlayer::PausedState);
        s.seek(Second);
        QCOMPARE(player->position(), qint64(1000));
        number(w, "brightness", 15);
        QCOMPARE(s.project().clips[0].visual.brightness, .15);
        s.edit([](Editor &e) { e.undo(); });
        s.seek(0);
        QTest::mouseClick(widget<QPushButton>(w, "addTitleButton"), Qt::LeftButton);
        QCOMPARE(s.project().clips.size(), size_t(2));
        auto id = s.selected;
        widget<QPlainTextEdit>(w, "titleText")->setPlainText("Perplex – Titel äöü");
        QTRY_COMPARE_WITH_TIMEOUT(s.project().clip(id).title.text,
                                  std::string("Perplex – Titel äöü"), 3000);
        number(w, "titleDuration", 1.5);
        QCOMPARE(s.project().clip(id).duration(), Time(1500000));
        number(w, "titleX", 30);
        number(w, "titleY", 25);
        QCOMPARE(s.project().clip(id).title.x, .3);
        number(w, "titleSize", 100);
        number(w, "visualFadeIn", .2);
        number(w, "visualFadeOut", .2);
        auto *format = widget<QComboBox>(w, "projectFormatBox");
        format->setCurrentIndex(1);
        QMetaObject::invokeMethod(format, "activated", Q_ARG(int, 1));
        QCOMPARE(s.project().settings.width, 1080);
        QTest::mouseClick(next, Qt::LeftButton);
        QTRY_VERIFY_WITH_TIMEOUT(play->isEnabled() && !player->source().isEmpty(), 30000);
        QTRY_COMPARE_WITH_TIMEOUT(preview->frameImage().size(), QSize(720, 1280), 5000);
        auto size = preview->frameImage().size().scaled(preview->size(), Qt::KeepAspectRatio);
        QPoint at((preview->width() - size.width()) / 2 + int(size.width() * .5),
                  (preview->height() - size.height()) / 2 + int(size.height() * .6));
        QTest::mouseClick(preview, Qt::LeftButton, Qt::NoModifier, at);
        QVERIFY(std::abs(s.project().clip(id).title.x - .5) < .01);
        QVERIFY(std::abs(s.project().clip(id).title.y - .6) < .01);
        QTRY_VERIFY_WITH_TIMEOUT(play->isEnabled() && !player->source().isEmpty(), 30000);
        s.seek(700000);
        QTest::qWait(200);
        if (qEnvironmentVariableIsSet("PMS_UI_CAPTURE")) {
            w.setLanguage("de-DE", false);
            QTest::qWait(200);
            QVERIFY(w.grab().save(qEnvironmentVariable("PMS_UI_CAPTURE")));
        }
    }
};
QTEST_MAIN(WindowTests)
#include "window_tests.moc"
