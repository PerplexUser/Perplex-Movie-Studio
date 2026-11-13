#include "media/MediaTools.h"
#include "domain/Project.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
namespace pms {
QString mediaTool(const QString &name) {
    auto env = qEnvironmentVariable(("PMS_" + name.toUpper()).toUtf8().constData());
    if (!env.isEmpty() && QFileInfo(env).isExecutable())
        return QFileInfo(env).absoluteFilePath();
    QString suffix;
#ifdef Q_OS_WIN
    suffix = ".exe";
#endif
    for (const auto &dir : {QCoreApplication::applicationDirPath() + "/media",
                            QCoreApplication::applicationDirPath()}) {
        auto path = dir + "/" + name + suffix;
        if (QFileInfo(path).isExecutable())
            return path;
    }
    return QStandardPaths::findExecutable(name + suffix);
}
QString seconds(qint64 us) {
    return QString::number(double(us) / Second, 'f', 6);
}
QString timecode(qint64 us, int num, int den) {
    us = std::max<qint64>(0, us);
    qint64 secs = us / Second;
    return QString("%1:%2:%3:%4")
        .arg(secs / 3600, 2, 10, QChar('0'))
        .arg(secs / 60 % 60, 2, 10, QChar('0'))
        .arg(secs % 60, 2, 10, QChar('0'))
        .arg((us % Second) * num / (Second * den), 2, 10, QChar('0'));
}
QStringList localInput(const QString &path) {
    return {"-protocol_whitelist", "file,pipe", "-i", QFileInfo(path).absoluteFilePath()};
}
QString errorText(int code) {
    const char *s = "The operation could not be completed.";
    switch (Error(code)) {
    case Error::InvalidProject:
        s = QT_TRANSLATE_NOOP("Errors", "The project is invalid or damaged.");
        break;
    case Error::UnsupportedSchema:
        s = QT_TRANSLATE_NOOP(
            "Errors", "This project uses an unsupported format version. It has not been changed.");
        break;
    case Error::MissingItem:
        s = QT_TRANSLATE_NOOP("Errors", "The selected item no longer exists.");
        break;
    case Error::LockedTrack:
        s = QT_TRANSLATE_NOOP("Errors", "This track is locked. Unlock it before editing.");
        break;
    case Error::Overlap:
        s = QT_TRANSLATE_NOOP("Errors", "Clips cannot overlap on the same track. Move the clip to "
                                        "an empty area or another track.");
        break;
    case Error::InvalidRange:
        s = QT_TRANSLATE_NOOP("Errors", "The time range or audio value is invalid. Split inside a "
                                        "clip and keep trims within the source.");
        break;
    case Error::WrongTrack:
        s = QT_TRANSLATE_NOOP("Errors",
                              "This media does not have a stream suitable for that track.");
        break;
    case Error::EmptyProject:
        s = QT_TRANSLATE_NOOP("Errors", "Add a clip to the timeline before exporting.");
        break;
    case Error::AssetInUse:
        return QCoreApplication::translate("Errors",
                                           "This media is used in the timeline. Remove its clips "
                                           "first. The original file will remain on disk.");
    case Error::LimitExceeded:
        s = QT_TRANSLATE_NOOP("Errors", "The project exceeds the supported size limit.");
        break;
    }
    return QCoreApplication::translate("Errors", s);
}
} // namespace pms
