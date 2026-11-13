#pragma once
#include "app/Session.h"
#include <QListWidget>
namespace pms {
class MediaBin : public QListWidget {
    Q_OBJECT
  public:
    explicit MediaBin(Session *, QWidget *parent = nullptr);
    void refresh();
    void removeSelected();
  signals:
    void removeRequested(const QString &assetId);
    void filesDropped(const QStringList &files);
    void addRequested(const QString &assetId);

  protected:
    void startDrag(Qt::DropActions) override;
    void dragEnterEvent(QDragEnterEvent *) override;
    void dragMoveEvent(QDragMoveEvent *) override;
    void dropEvent(QDropEvent *) override;

  private:
    Session *session_;
};
} // namespace pms
