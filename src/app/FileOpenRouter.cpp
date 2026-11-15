#include "app/FileOpenRouter.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QTimer>

namespace pms {
FileOpenRouter::FileOpenRouter(QObject *parent) : QObject(parent) {
    QCoreApplication::instance()->installEventFilter(this);
}
void FileOpenRouter::deliver(const QString &path) {
    QTimer::singleShot(0, this, [this, path] { emit projectRequested(path); });
}
void FileOpenRouter::setReady() {
    if (ready_)
        return;
    ready_ = true;
    const auto pending = pending_;
    pending_.clear();
    for (const auto &path : pending)
        deliver(path);
}
bool FileOpenRouter::eventFilter(QObject *object, QEvent *event) {
    if (event->type() != QEvent::FileOpen)
        return QObject::eventFilter(object, event);
    auto *open = static_cast<QFileOpenEvent *>(event);
    const auto url = open->url();
    if (!url.isLocalFile() ||
        QFileInfo(url.toLocalFile()).suffix().compare("pmsproj", Qt::CaseInsensitive))
        return false;
    received_ = true;
    const auto path = QFileInfo(url.toLocalFile()).absoluteFilePath();
    if (ready_)
        deliver(path);
    else if (pending_.size() < 32)
        pending_.append(path);
    open->accept();
    return true;
}
} // namespace pms
