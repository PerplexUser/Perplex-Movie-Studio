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
#include <QTextDocument>
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
                     "Perplex Movie Studio\nVersion 1.0.0\n\n© by www.perplex.click\nMichael "
                     "Lappenbusch\n\nEigener Anwendungscode: MIT-Lizenz.\nQt: © The Qt Company "
                     "Ltd. und Mitwirkende, LGPL v3.\nFFmpeg: LGPL v2.1 oder neuer im "
                     "mitgelieferten Build.\nLAME: LGPL v2 oder neuer.\n\nSiehe LICENSE und "
                     "THIRD_PARTY_NOTICES.md neben der Anwendung."));
    }
    void typingKeepsCursorAndSelection() {
        MainWindow w(true);
        w.show();
        auto &s = w.session();
        QTest::mouseClick(widget<QPushButton>(w, "addTitleButton"), Qt::LeftButton);
        const auto id = s.selected;
        auto *text = widget<QPlainTextEdit>(w, "titleText");
        QString expected;
        for (char letter : {'b', 'l', 'a'}) {
            QTest::keyClicks(text, QString(QChar(letter)));
            expected += QChar(letter);
            // Wait for the delayed model commit between individual key presses.
            QTRY_COMPARE_WITH_TIMEOUT(s.project().clip(id).title.text, expected.toStdString(),
                                      3000);
            QCOMPARE(text->toPlainText(), expected);
            QCOMPARE(text->textCursor().position(), expected.size());
        }
        auto cursor = text->textCursor();
        cursor.setPosition(1);
        text->setTextCursor(cursor);
        text->insertPlainText(QString::fromUtf8("ä"));
        QTRY_COMPARE_WITH_TIMEOUT(s.project().clip(id).title.text, std::string("bäla"), 3000);
        QCOMPARE(text->textCursor().position(), 2);
        cursor = text->textCursor();
        cursor.setPosition(3);
        cursor.setPosition(1, QTextCursor::KeepAnchor);
        text->setTextCursor(cursor);
        number(w, "titleX", 30);
        QCOMPARE(text->textCursor().anchor(), 3);
        QCOMPARE(text->textCursor().position(), 1);
        QCOMPARE(text->textCursor().selectedText(), QString::fromUtf8("äl"));
        text->insertPlainText("XY");
        QTRY_COMPARE_WITH_TIMEOUT(s.project().clip(id).title.text, std::string("bXYa"), 3000);
        text->undo();
        QTRY_COMPARE_WITH_TIMEOUT(s.project().clip(id).title.text, std::string("bäla"), 3000);
        text->redo();
        QTRY_COMPARE_WITH_TIMEOUT(s.project().clip(id).title.text, std::string("bXYa"), 3000);

        // Changing selection while a commit is pending keeps both titles intact.
        text->insertPlainText("!");
        const auto pending = text->toPlainText().toStdString();
        s.seek(s.project().duration());
        QTest::mouseClick(widget<QPushButton>(w, "addTitleButton"), Qt::LeftButton);
        const auto second = s.selected;
        QVERIFY(second != id);
        QCOMPARE(s.project().clip(id).title.text, pending);
        QTest::keyClicks(text, "second");
        s.select(id);
        QCOMPARE(s.project().clip(second).title.text, std::string("second"));
        QCOMPARE(text->toPlainText().toStdString(), pending);
        const auto oldText = s.project().clip(id).title;
        auto replacement = oldText;
        replacement.text = "Replacement";
        s.edit([&](Editor &e) { e.title(id, replacement); });
        QCOMPARE(text->toPlainText(), QString("Replacement"));
        s.edit([](Editor &e) { e.undo(); });
        QCOMPARE(text->toPlainText().toStdString(), oldText.text);
        // Equal text on a different clip must not inherit this clip's local undo.
        text->insertPlainText("q");
        const auto matched = text->toPlainText().toStdString();
        QTRY_COMPARE_WITH_TIMEOUT(s.project().clip(id).title.text, matched, 3000);
        QVERIFY(text->document()->isUndoAvailable());
        auto matching = s.project().clip(second).title;
        matching.text = matched;
        s.edit([&](Editor &e) { e.title(second, matching); });
        s.select(second);
        QVERIFY(!text->document()->isUndoAvailable());
        text->undo();
        QCOMPARE(text->toPlainText().toStdString(), matched);
    }
    void opacityPreviewAndLock() {
        MainWindow w(true);
        w.show();
        auto &s = w.session();
        w.importFiles({m.color("opacity-ui-red", "red"), m.color("opacity-ui-blue", "blue")});
        QTRY_COMPARE_WITH_TIMEOUT(s.project().assets.size(), size_t(2), 10000);
        s.edit([&](Editor &e) {
            auto lower = e.addClip(e.project().assets[0].id, e.project().tracks[0].id, 0);
            e.trim(lower, 0, 400000, 0);
            s.selected = e.addClip(e.project().assets[1].id, e.project().tracks[1].id, 0);
            e.trim(s.selected, 0, 400000, 0);
        });
        auto *opacity = widget<QDoubleSpinBox>(w, "opacity");
        QCOMPARE(opacity->value(), 100.0);
        number(w, "opacity", 50);
        QCOMPARE(s.project().clip(s.selected).visual.opacity, .5);
        auto *player = w.findChild<QMediaPlayer *>();
        QVERIFY(player && player->isAvailable());
        w.findChild<QAudioOutput *>()->setVolume(0);
        auto *preview = widget<PreviewWidget>(w, "videoPreview");
        auto *next = widget<QPushButton>(w, "nextFrameButton");
        QTest::mouseClick(next, Qt::LeftButton);
        QTRY_VERIFY_WITH_TIMEOUT(preview->hasFrame(), 30000);
        auto image = preview->frameImage();
        auto pixel = image.pixelColor(image.width() / 2, image.height() / 2);
        QVERIFY(pixel.red() > 95 && pixel.red() < 165 && pixel.blue() > 95 && pixel.blue() < 165);
        QVERIFY(pixel.green() < 25);
        number(w, "opacity", 0);
        QTest::mouseClick(next, Qt::LeftButton);
        QTRY_VERIFY_WITH_TIMEOUT(
            preview->hasFrame() && preview->frameImage().pixelColor(640, 360).red() > 220, 30000);
        image = preview->frameImage();
        pixel = image.pixelColor(image.width() / 2, image.height() / 2);
        QVERIFY(pixel.red() > 220 && pixel.blue() < 25);
        s.edit([](Editor &e) { e.undo(); });
        QCOMPARE(opacity->value(), 50.0);
        const auto track = s.project().clip(s.selected).trackId;
        s.edit([&](Editor &e) { e.trackState(track, false, true); });
        QVERIFY(!opacity->isEnabled());
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
        QVERIFY2(player->isAvailable(), "The packaged Qt media backend could not be loaded");
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
