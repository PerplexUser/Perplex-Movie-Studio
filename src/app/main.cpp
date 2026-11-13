#include "export/RenderJob.h"
#include "media/ProbeJob.h"
#include "persistence/ProjectStore.h"
#include "ui/MainWindow.h"
#include "ui/Theme.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QStyleFactory>
#include <QTextStream>
#include <QTimer>

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setApplicationName("PerplexMovieStudio");
    app.setOrganizationName("perplex.click");
    app.setApplicationVersion(PMS_VERSION);
    app.setStyle(QStyleFactory::create("Fusion"));
    app.setStyleSheet(pms::Theme);
    QCommandLineParser parser;
    parser.setApplicationDescription("Perplex Movie Studio — local nonlinear video editor");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({"self-test-ui", "Run the desktop smoke test and exit."});
    parser.addOption({"screenshot", "Save a desktop screenshot for layout verification.", "path"});
    parser.addOption({"language", "Use en-US or de-DE.", "locale"});
    parser.addOption({"probe", "Probe a local media file and output JSON.", "file"});
    parser.addOption({"render", "Render a project from the command line.", "project"});
    parser.addOption({"output", "Render destination (extension selects format).", "file"});
    parser.addPositionalArgument("project", "Optional .pmsproj project to open.");
    parser.process(app);
    if (parser.isSet("probe")) {
        pms::ProbeJob job;
        QObject::connect(&job, &pms::ProbeJob::succeeded, &app, [&](const pms::Asset &a) {
            auto p = pms::makeProject();
            p.assets.push_back(a);
            QTextStream(stdout) << QJsonDocument(pms::ProjectStore::encode(p, QDir::currentPath() +
                                                                                  "/probe.pmsproj"))
                                       .toJson();
            app.exit(0);
        });
        QObject::connect(&job, &pms::ProbeJob::failed, &app,
                         [&](const QString &msg, const QString &detail) {
                             QTextStream(stderr) << msg << '\n' << detail << '\n';
                             app.exit(2);
                         });
        job.start(parser.value("probe"));
        return app.exec();
    }
    if (parser.isSet("render")) {
        try {
            auto p = pms::ProjectStore::load(parser.value("render"));
            auto path = parser.value("output");
            if (path.isEmpty())
                return 2;
            pms::RenderProfile profile;
            profile.format = QFileInfo(path).suffix().toLower();
            profile.width = p.settings.width;
            profile.height = p.settings.height;
            profile.fps = p.settings.fpsNum;
            pms::RenderJob job;
            QObject::connect(&job, &pms::RenderJob::succeeded, &app,
                             [&](const auto &path, const auto &encoder) {
                                 QTextStream(stdout) << path << " (" << encoder << ")\n";
                                 app.exit(0);
                             });
            QObject::connect(&job, &pms::RenderJob::failed, &app,
                             [&](const auto &msg, const auto &detail) {
                                 QTextStream(stderr) << msg << '\n' << detail << '\n';
                                 app.exit(2);
                             });
            job.start(p, profile, path, false);
            return app.exec();
        } catch (const std::exception &e) {
            QTextStream(stderr) << e.what() << '\n';
            return 2;
        }
    }
    const bool test = parser.isSet("self-test-ui") || parser.isSet("screenshot");
    pms::MainWindow window(test);
    if (parser.isSet("language"))
        window.setLanguage(parser.value("language"), !test);
    if (!parser.positionalArguments().isEmpty())
        window.openProject(parser.positionalArguments().first());
    window.show();
    if (parser.isSet("self-test-ui"))
        QTimer::singleShot(250, &app, [&] {
            bool ok = window.smokeTest();
            QTextStream(stdout) << (ok ? "PASS UI widgets and English/German translation\n"
                                       : "FAIL UI smoke test\n");
            app.exit(ok ? 0 : 1);
        });
    else if (parser.isSet("screenshot"))
        QTimer::singleShot(
            700, &app, [&] { app.exit(window.grab().save(parser.value("screenshot")) ? 0 : 1); });
    else
        QTimer::singleShot(100, &window, [&] {
            if (parser.positionalArguments().isEmpty())
                window.recover();
        });
    return app.exec();
}
