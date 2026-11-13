#include "TestMedia.h"
#include "persistence/ProjectStore.h"
#include <QPainter>
#include <array>
using namespace pms;
class FeatureTests : public QObject {
    Q_OBJECT
    TestMedia m;
    QString red, blue;
    static bool near(QColor a, QColor b, int d = 25) {
        return std::abs(a.red() - b.red()) <= d && std::abs(a.green() - b.green()) <= d &&
               std::abs(a.blue() - b.blue()) <= d;
    }
    static int white(const QImage &im, int a, int b) {
        int n = 0;
        for (int y = a; y < b; ++y)
            for (int x = 0; x < im.width(); ++x) {
                auto c = im.pixelColor(x, y);
                if (c.red() > 160 && c.green() > 160 && c.blue() > 160)
                    ++n;
            }
        return n;
    }
  private slots:
    void initTestCase() {
        qRegisterMetaType<Asset>();
        red = m.color("red", "red");
        blue = m.color("blue", "blue");
    }
    void fullFrameColors_data() {
        QTest::addColumn<int>("width");
        QTest::addColumn<int>("height");
        for (auto sz : {QSize(320, 180), QSize(1280, 720), QSize(1920, 1080), QSize(180, 320),
                        QSize(720, 1280), QSize(1080, 1920)})
            QTest::newRow(qPrintable(QString("%1x%2").arg(sz.width()).arg(sz.height())))
                << sz.width() << sz.height();
    }
    void fullFrameColors() {
        QFETCH(int, width);
        QFETCH(int, height);
        int w = width > height ? 320 : 180, h = width > height ? 180 : 320;
        QImage im(w, h, QImage::Format_RGB32);
        QPainter p(&im);
        QColor colors[] = {Qt::red, Qt::green, Qt::blue, Qt::white};
        for (int i = 0; i < 4; ++i)
            p.fillRect(i % 2 * w / 2, i / 2 * h / 2, w / 2, h / 2, colors[i]);
        p.end();
        auto png = m.path("quad.png");
        QVERIFY(im.save(png));
        auto e = m.edit(png, 400000);
        e.settings({width, height, 30, 1});
        auto output = m.render(e.project(), width, height);
        auto video = m.edit(output, 400000);
        video.settings({width, height, 30, 1});
        auto frame = m.frame(m.render(video.project(), width, height), .2, width, height);
        for (int y = 4; y < height - 4; ++y) {
            if (std::abs(y - height / 2) < 6 * height / h)
                continue;
            for (int x : {width / 8, width * 3 / 8, width * 5 / 8, width * 7 / 8})
                QVERIFY2(near(frame.pixelColor(x, y),
                              colors[(y >= height / 2 ? 2 : 0) + (x >= width / 2 ? 1 : 0)]),
                         qPrintable(QString("Pixel %1,%2: %3")
                                        .arg(x)
                                        .arg(y)
                                        .arg(frame.pixelColor(x, y).name())));
        }
    }
    void phoneRotation() {
        auto f = m.path("phone.mp4");
        m.run({"-v", "error", "-y", "-display_rotation", "90", "-i", red, "-c", "copy", f});
        auto e = m.edit(f, 400000);
        QCOMPARE(std::abs(e.project().assets[0].rotation), 90);
        e.settings({1080, 1920, 30, 1});
        auto im = m.frame(m.render(e.project(), 180, 320), .2, 180, 320);
        QVERIFY(near(im.pixelColor(10, 10), Qt::red));
        QVERIFY(near(im.pixelColor(170, 310), Qt::red));
    }
    void imagesAndText() {
        QImage im(320, 180, QImage::Format_ARGB32);
        im.fill(Qt::transparent);
        QPainter p(&im);
        p.fillRect(80, 40, 160, 100, Qt::blue);
        p.end();
        auto png = m.path("Bïld.png");
        QVERIFY(im.save(png));
        auto e = m.edit(red);
        auto a = e.addAsset(m.probe(png));
        QCOMPARE(e.project().asset(a).kind, AssetKind::Image);
        auto c = e.addClip(a, e.project().tracks[1].id, Second / 2);
        e.trim(c, 0, Second, Second / 2);
        auto f = m.render(e.project());
        QVERIFY(near(m.frame(f, .2).pixelColor(160, 90), Qt::red));
        QVERIFY(near(m.frame(f, .8).pixelColor(160, 90), Qt::blue));
        QVERIFY(near(m.frame(f, .8).pixelColor(10, 10), Qt::red));
        QVERIFY(near(m.frame(f, 1.8).pixelColor(160, 90), Qt::red));
        QImage photo(320, 180, QImage::Format_RGB32);
        photo.fill(Qt::green);
        auto jpg = m.path("photo.jpg");
        QVERIFY(photo.save(jpg));
        auto still = m.edit(jpg, Second);
        auto id = still.project().clips[0].id;
        auto v = still.project().clip(id).visual;
        v.fadeIn = Second / 2;
        v.fadeOut = Second / 2;
        still.visual(id, v);
        f = m.render(still.project());
        QVERIFY(m.frame(f, .1).pixelColor(160, 90).green() < 100);
        QVERIFY(m.frame(f, .5).pixelColor(160, 90).green() > 220);
        QVERIFY(m.frame(f, .9).pixelColor(160, 90).green() < 100);
        e = m.edit(red);
        id = e.addTitle(Second / 2, "Grüße: 'A;B' 100%\\\nZeile 2");
        e.trim(id, 0, Second, Second / 2);
        auto t = e.project().clip(id).title;
        t.y = .25;
        t.size = 28;
        e.title(id, t);
        v = e.project().clip(id).visual;
        v.fadeIn = 200000;
        v.fadeOut = 200000;
        e.visual(id, v);
        f = m.render(e.project());
        QCOMPARE(white(m.frame(f, .2), 0, 180), 0);
        auto shown = m.frame(f, .9);
        QVERIFY(white(shown, 0, 90) > 100);
        QCOMPARE(white(shown, 100, 180), 0);
        QCOMPARE(white(m.frame(f, 1.8), 0, 180), 0);
        QVERIFY(white(m.frame(f, .53), 0, 180) < white(shown, 0, 180) / 2);
        t.y = .75;
        e.title(id, t);
        auto lower = m.frame(m.render(e.project()), .9);
        QVERIFY(white(lower, 90, 180) > 100);
        QCOMPARE(white(lower, 0, 70), 0);
    }
    void corrections() {
        QImage im(320, 180, QImage::Format_RGB32);
        im.fill(QColor(96, 96, 96));
        auto png = m.path("gray.png");
        QVERIFY(im.save(png));
        auto e = m.edit(png, 400000);
        auto c = e.project().clips[0];
        for (auto v : {std::array<double, 3>{.2, 1, 147}, {-.2, 1, 45}, {0, 2, 64}, {0, .5, 112}}) {
            c.visual.brightness = v[0];
            c.visual.contrast = v[1];
            e.visual(c.id, c.visual);
            QVERIFY(near(m.frame(m.render(e.project()), .2).pixelColor(160, 90),
                         QColor(int(v[2]), int(v[2]), int(v[2])), 8));
        }
        e = m.edit(red, 400000);
        c = e.project().clips[0];
        c.visual.saturation = 0;
        e.visual(c.id, c.visual);
        auto gray = m.frame(m.render(e.project()), .2).pixelColor(160, 90);
        QVERIFY(std::abs(gray.red() - gray.blue()) < 5);
        QVERIFY(std::abs(gray.green() - gray.blue()) < 5);
        c.visual.saturation = 1;
        c.visual.hue = 180;
        e.visual(c.id, c.visual);
        auto turned = m.frame(m.render(e.project()), .2).pixelColor(160, 90);
        QVERIFY(turned.green() > turned.red() + 50 && turned.blue() > turned.red() + 50);
    }
    void transitions_data() {
        QTest::addColumn<int>("mode");
        QTest::newRow("fade") << 0;
        QTest::newRow("wipe") << 1;
        QTest::newRow("slide") << 2;
    }
    void transitions() {
        QFETCH(int, mode);
        auto e = m.edit(red);
        auto a = e.addAsset(m.probe(blue));
        auto id = e.addClip(a, e.project().tracks[1].id, 0);
        auto v = e.project().clip(id).visual;
        v.transition = Transition(mode);
        v.fadeIn = Second;
        v.fadeOut = Second;
        e.visual(id, v);
        auto f = m.render(e.project());
        auto early = m.frame(f, .2), late = m.frame(f, .8), out = m.frame(f, 1.8);
        if (mode == 0) {
            QVERIFY(early.pixelColor(160, 90).red() > 150);
            QVERIFY(late.pixelColor(160, 90).blue() > 150);
            QVERIFY(out.pixelColor(160, 90).red() > 150);
        } else {
            QVERIFY(near(early.pixelColor(160, 90), Qt::red));
            QVERIFY(near(late.pixelColor(160, 90), Qt::blue));
            QVERIFY(near(out.pixelColor(160, 90), Qt::red));
        }
        e.split(id, Second / 2);
        auto split = m.frame(m.render(e.project()), .8);
        for (int y = 5; y < 175; y += 13)
            for (int x = 5; x < 315; x += 13)
                QVERIFY(near(split.pixelColor(x, y), late.pixelColor(x, y), 10));
    }
    void persistenceAndAtomicOutput() {
        auto e = m.edit(red, 400000);
        auto id = e.addTitle(0, "Titel äöü");
        auto c = e.project().clip(id);
        c.title.x = .2;
        e.title(id, c.title);
        c.visual.brightness = .2;
        c.visual.fadeIn = Second;
        e.visual(id, c.visual);
        e.settings({1080, 1920, 30, 1});
        auto path = m.path("test.pmsproj");
        ProjectStore::save(e.project(), path);
        QCOMPARE(ProjectStore::load(path), e.project());
        QVERIFY(ProjectStore::missing(e.project()).isEmpty());
        auto before = e.project();
        auto invalid = c.title;
        invalid.x = 2;
        QVERIFY_EXCEPTION_THROWN(e.title(id, invalid), DomainError);
        QVERIFY_EXCEPTION_THROWN(e.removeAsset(c.assetId), DomainError);
        QCOMPARE(e.project(), before);
        e.erase(id);
        e.removeAsset(c.assetId);
        e.undo();
        e.undo();
        QCOMPARE(e.project(), before);
        e.trackState(c.trackId, false, true);
        QVERIFY_EXCEPTION_THROWN(e.title(id, c.title), DomainError);
        auto legacy = ProjectStore::encode(m.edit(red).project(), path);
        legacy["schemaVersion"] = 1;
        auto assets = legacy["assets"].toArray();
        for (int i = 0; i < assets.size(); ++i) {
            auto o = assets[i].toObject();
            o.remove("kind");
            assets[i] = o;
        }
        legacy["assets"] = assets;
        auto clips = legacy["clips"].toArray();
        for (int i = 0; i < clips.size(); ++i) {
            auto o = clips[i].toObject();
            o.remove("title");
            o.remove("visual");
            clips[i] = o;
        }
        legacy["clips"] = clips;
        auto migrated = ProjectStore::decode(legacy, path);
        QCOMPARE(migrated.schemaVersion, 2);
        QCOMPARE(migrated.clips[0].visual.brightness, 0.0);
        e = m.edit(red, 400000);
        path = m.path("overwrite € file.mp4");
        {
            QFile f(path);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("old");
        }
        RenderProfile profile;
        profile.width = 320;
        profile.height = 180;
        RenderJob denied;
        QSignalSpy bad(&denied, &RenderJob::failed);
        denied.start(e.project(), profile, path, false);
        QVERIFY(bad.wait(5000));
        {
            QFile f(path);
            QVERIFY(f.open(QIODevice::ReadOnly));
            QCOMPARE(f.readAll(), QByteArray("old"));
        }
        m.render(e.project(), 320, 180, path);
        QVERIFY(near(m.frame(path, .2).pixelColor(160, 90), Qt::red));
        auto blocked = m.path("blocked.mp4");
        QVERIFY(QDir().mkdir(blocked));
        QFile sentinel(blocked + "/keep");
        QVERIFY(sentinel.open(QIODevice::WriteOnly));
        sentinel.write("keep");
        sentinel.close();
        RenderJob job;
        QSignalSpy failed(&job, &RenderJob::failed);
        job.start(e.project(), profile, blocked, true);
        QVERIFY(failed.wait(10000));
        QVERIFY(QFileInfo::exists(sentinel.fileName()));
        QVERIFY(QDir(m.directory.path())
                    .entryList({".pms-export-*"}, QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot)
                    .isEmpty());
    }
};
QTEST_MAIN(FeatureTests)
#include "feature_tests.moc"
