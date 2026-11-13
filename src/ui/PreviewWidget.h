#pragma once
#include <QImage>
#include <QVideoSink>
#include <QWidget>
namespace pms {
bool configureSoftwarePlayback();
class PreviewWidget final : public QWidget {
    Q_OBJECT
  public:
    explicit PreviewWidget(QWidget *parent = nullptr);
    QVideoSink *videoSink() {
        return &sink_;
    }
    bool hasFrame() const {
        return !image_.isNull();
    }
    const QImage &frameImage() const {
        return image_;
    }
    void clear();
    void setPlacementEnabled(bool);
  signals:
    void textPositionRequested(double x, double y);

  protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

  private:
    QRectF imageRect() const;
    QVideoSink sink_;
    QImage image_;
    bool placement_ = false, dragging_ = false;
    QPointF pointer_;
};
} // namespace pms
