#pragma once
#include <QObject>
#include <QStringList>

namespace pms {
// Finder can send document-open events before the main window is ready.
class FileOpenRouter final : public QObject {
    Q_OBJECT
  public:
    explicit FileOpenRouter(QObject *parent = nullptr);
    void setReady();
    bool hasRequests() const {
        return received_;
    }
  signals:
    void projectRequested(const QString &path);

  protected:
    bool eventFilter(QObject *, QEvent *) override;

  private:
    void deliver(const QString &path);
    QStringList pending_;
    bool ready_ = false, received_ = false;
};
} // namespace pms
