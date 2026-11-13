#pragma once
#include "app/Session.h"
#include <QAbstractScrollArea>
namespace pms {
class Timeline final : public QAbstractScrollArea {
    Q_OBJECT
  public:
    explicit Timeline(Session *, QWidget *parent = nullptr);
    void zoom(double factor);
    void fit();
    void setSnapping(bool value) {
        snapping_ = value;
    }
  signals:
    void filesDropped(const QStringList &files);

  protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void dragEnterEvent(QDragEnterEvent *) override;
    void dragMoveEvent(QDragMoveEvent *) override;
    void dropEvent(QDropEvent *) override;
    void contextMenuEvent(QContextMenuEvent *) override;

  private:
    void refresh();
    std::vector<Track> tracks() const;
    QRectF rect(const Clip &) const;
    Time timeAt(double x) const;
    double xAt(Time) const;
    int rowAt(double y) const;
    Time snapped(Time candidate, const std::string &excluded) const;
    Session *session_;
    double scale_ = 64;
    bool snapping_ = true, scrubbing_ = false;
    std::optional<Clip> original_, ghost_;
    QPointF press_;
    int mode_ = 0;
    static constexpr int Header = 146, Ruler = 38, Row = 60;
};
} // namespace pms
