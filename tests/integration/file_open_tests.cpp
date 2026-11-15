#include "app/FileOpenRouter.h"
#include <QDir>
#include <QFileOpenEvent>
#include <QSignalSpy>
#include <QtTest>

class FileOpenTests : public QObject {
    Q_OBJECT
  private slots:
    void queuesEarlyEventsAndOpensLaterDocuments() {
        pms::FileOpenRouter router;
        QSignalSpy requests(&router, &pms::FileOpenRouter::projectRequested);
        const auto first = QDir::tempPath() + QString::fromUtf8("/Mein Film ä.PMSPROJ");
        QFileOpenEvent early(first);
        QCoreApplication::sendEvent(qApp, &early);
        QVERIFY(router.hasRequests());
        QCOMPARE(requests.count(), 0);
        router.setReady();
        QTRY_COMPARE(requests.count(), 1);
        QCOMPARE(requests.at(0).at(0).toString(), first);
        router.setReady();
        QFileOpenEvent later(QUrl::fromLocalFile(QDir::tempPath() + "/second.pmsproj"));
        QCoreApplication::sendEvent(qApp, &later);
        QTRY_COMPARE(requests.count(), 2);
    }
    void rejectsRemoteUrlsAndOtherFiles() {
        pms::FileOpenRouter router;
        QSignalSpy requests(&router, &pms::FileOpenRouter::projectRequested);
        router.setReady();
        QFileOpenEvent remote(QUrl("https://example.invalid/test.pmsproj"));
        QFileOpenEvent image(QDir::tempPath() + "/image.png");
        QCoreApplication::sendEvent(qApp, &remote);
        QCoreApplication::sendEvent(qApp, &image);
        QCoreApplication::processEvents();
        QVERIFY(!router.hasRequests());
        QVERIFY(requests.isEmpty());
    }
};
QTEST_MAIN(FileOpenTests)
#include "file_open_tests.moc"
