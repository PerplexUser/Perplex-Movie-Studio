#include "ui/PreviewWidget.h"
#include <QCoreApplication>
#include <QDebug>
#include <QLibrary>
#include <QLibraryInfo>
#include <QMouseEvent>
#include <QPainter>
#include <QVideoFrame>
#include <algorithm>
namespace pms {
bool configureSoftwarePlayback() {
    // The reviewed FFmpeg build's partial SIMD scaler corrupts chroma. Keep
    // both the command-line renderer and Qt's loaded libavutil on scalar paths.
    QStringList paths = {QCoreApplication::applicationDirPath() + "/avutil-59.dll", "avutil-59",
                         QLibraryInfo::path(QLibraryInfo::LibrariesPath) + "/libavutil.so.59",
                         "libavutil.so.59"};
    for (const auto &path : paths) {
        QLibrary lib(path);
        lib.setLoadHints(QLibrary::PreventUnloadHint);
        if (!lib.load())
            continue;
        auto force = reinterpret_cast<void (*)(int)>(lib.resolve("av_force_cpu_flags"));
        auto flags = reinterpret_cast<int (*)()>(lib.resolve("av_get_cpu_flags"));
        if (force && flags) {
            force(0);
            return flags() == 0;
        }
    }
    return false;
}
PreviewWidget::PreviewWidget(QWidget *parent) : QWidget(parent), sink_(this) {
    setObjectName("videoPreview");
    setMinimumSize(100, 100);
    if (!configureSoftwarePlayback())
        qWarning() << "Software pixel policy could not load libavutil";
    connect(&sink_, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame &frame) {
        if (!frame.isValid())
            return;
        auto image = frame.toImage();
        if (!image.isNull()) {
            image_ = image;
            update();
        }
    });
}
void PreviewWidget::clear() {
    image_ = {};
    update();
}
void PreviewWidget::setPlacementEnabled(bool enabled) {
    placement_ = enabled;
    dragging_ = false;
    setCursor(enabled ? Qt::CrossCursor : Qt::ArrowCursor);
    update();
}
QRectF PreviewWidget::imageRect() const {
    if (image_.isNull())
        return {};
    auto s = image_.size().scaled(size(), Qt::KeepAspectRatio);
    return {(width() - s.width()) / 2.0, (height() - s.height()) / 2.0, double(s.width()),
            double(s.height())};
}
void PreviewWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), Qt::black);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    if (!image_.isNull())
        p.drawImage(imageRect(), image_);
    if (dragging_) {
        p.setPen(QPen(QColor("#84e1bc"), 2));
        p.drawLine(pointer_ + QPointF(-16, 0), pointer_ + QPointF(16, 0));
        p.drawLine(pointer_ + QPointF(0, -16), pointer_ + QPointF(0, 16));
    }
}
void PreviewWidget::mousePressEvent(QMouseEvent *e) {
    if (placement_ && e->button() == Qt::LeftButton && imageRect().contains(e->position())) {
        dragging_ = true;
        pointer_ = e->position();
        update();
        e->accept();
    }
}
void PreviewWidget::mouseMoveEvent(QMouseEvent *e) {
    if (dragging_) {
        pointer_ = e->position();
        update();
    }
}
void PreviewWidget::mouseReleaseEvent(QMouseEvent *e) {
    if (!dragging_ || e->button() != Qt::LeftButton)
        return;
    dragging_ = false;
    auto r = imageRect();
    if (r.width() > 0 && r.height() > 0)
        emit textPositionRequested(std::clamp((e->position().x() - r.x()) / r.width(), 0.0, 1.0),
                                   std::clamp((e->position().y() - r.y()) / r.height(), 0.0, 1.0));
    update();
}
} // namespace pms
