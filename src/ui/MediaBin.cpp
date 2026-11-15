#include "ui/MediaBin.h"
#include "media/MediaTools.h"
#include <QDrag>
#include <QDropEvent>
#include <QFileInfo>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QUrl>
namespace pms {
MediaBin::MediaBin(Session *s, QWidget *parent) : QListWidget(parent), session_(s) {
    setObjectName("mediaBin");
    setDragEnabled(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        auto *item = itemAt(pos);
        if (!item)
            return;
        setCurrentItem(item);
        QMenu menu(this);
        auto *action = menu.addAction(tr("Remove from project"));
        if (menu.exec(viewport()->mapToGlobal(pos)) == action)
            removeSelected();
    });
    setAcceptDrops(true);
    setIconSize({72, 48});
    setSpacing(5);
    setWordWrap(true);
    setAccessibleName(tr("Media library"));
    connect(s, &Session::changed, this, &MediaBin::refresh);
    connect(this, &QListWidget::itemDoubleClicked, this,
            [this](auto *item) { emit addRequested(item->data(Qt::UserRole).toString()); });
    refresh();
}
void MediaBin::refresh() {
    QString selected = currentItem() ? currentItem()->data(Qt::UserRole).toString() : QString();
    clear();
    for (const auto &a : session_->project().assets) {
        bool missing =
            a.kind != AssetKind::Title && !QFileInfo(QString::fromStdString(a.path)).isFile();
        bool video = a.videoStream >= 0;
        QPixmap pix(144, 96);
        pix.fill(QColor(video ? "#244e45" : "#293e61"));
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor(video ? "#85dcc1" : "#9dc2ff"), 3));
        p.setBrush(Qt::NoBrush);
        if (video) {
            p.drawRoundedRect(QRectF(40, 26, 52, 40), 5, 5);
            p.drawPolygon(QPolygonF{{96, 38}, {111, 30}, {111, 63}, {96, 55}});
        } else {
            p.drawLine(65, 28, 65, 65);
            p.drawLine(65, 28, 91, 22);
            p.drawLine(91, 22, 91, 59);
            p.drawEllipse(QPointF(56, 65), 9, 6);
            p.drawEllipse(QPointF(82, 59), 9, 6);
        }
        p.end();
        pix.setDevicePixelRatio(2);
        auto *item =
            new QListWidgetItem(QIcon(pix),
                                QString::fromStdString(a.name) + "\n" +
                                    (a.kind == AssetKind::Title   ? tr("Text")
                                     : a.kind == AssetKind::Image ? tr("Image")
                                                                  : timecode(a.duration)) +
                                    "  ·  " +
                                    (missing ? tr("Missing")
                                     : video ? QString("%1 × %2").arg(a.width).arg(a.height)
                                             : tr("Audio")),
                                this);
        item->setData(Qt::UserRole, QString::fromStdString(a.id));
        item->setToolTip(
            QString::fromStdString(a.path) + "\n" + QString::fromStdString(a.container) + " · " +
            QString::fromStdString(a.videoCodec) + " / " + QString::fromStdString(a.audioCodec) +
            "\n" + tr("Drag to a track, or double-click to append."));
        item->setSizeHint({220, 68});
        if (item->data(Qt::UserRole).toString() == selected)
            setCurrentItem(item);
    }
}
void MediaBin::startDrag(Qt::DropActions) {
    if (!currentItem())
        return;
    auto *mime = new QMimeData;
    mime->setData("application/x-perplex-asset",
                  currentItem()->data(Qt::UserRole).toString().toUtf8());
    auto *drag = new QDrag(this);
    drag->setMimeData(mime);
    drag->setPixmap(currentItem()->icon().pixmap(72, 48));
    drag->exec(Qt::CopyAction);
}
void MediaBin::dragEnterEvent(QDragEnterEvent *e) {
    if (e->mimeData()->hasUrls())
        e->acceptProposedAction();
}
void MediaBin::dragMoveEvent(QDragMoveEvent *e) {
    if (e->mimeData()->hasUrls())
        e->acceptProposedAction();
}
void MediaBin::dropEvent(QDropEvent *e) {
    QStringList paths;
    for (auto u : e->mimeData()->urls())
        if (u.isLocalFile())
            paths << u.toLocalFile();
    emit filesDropped(paths);
    e->acceptProposedAction();
}
void MediaBin::removeSelected() {
    if (currentItem())
        emit removeRequested(currentItem()->data(Qt::UserRole).toString());
}
} // namespace pms
