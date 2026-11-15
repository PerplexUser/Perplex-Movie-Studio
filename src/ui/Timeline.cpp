#include "ui/Timeline.h"
#include "media/MediaTools.h"
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QToolTip>
#include <QUrl>
#include <QWheelEvent>
#include <cmath>
namespace pms {
Timeline::Timeline(Session *s, QWidget *parent) : QAbstractScrollArea(parent), session_(s) {
    setObjectName("timeline");
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);
    viewport()->setMouseTracking(true);
    setMinimumHeight(230);
    setAccessibleName(tr("Multitrack timeline"));
    connect(s, &Session::changed, this, &Timeline::refresh);
    connect(s, &Session::selectionChanged, viewport(), qOverload<>(&QWidget::update));
    connect(s, &Session::positionChanged, viewport(), qOverload<>(&QWidget::update));
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, viewport(),
            qOverload<>(&QWidget::update));
    connect(verticalScrollBar(), &QScrollBar::valueChanged, viewport(),
            qOverload<>(&QWidget::update));
    refresh();
}
std::vector<Track> Timeline::tracks() const {
    auto result = session_->project().tracks;
    std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) {
        if (a.type != b.type)
            return a.type == TrackType::Video;
        return a.type == TrackType::Video ? a.order > b.order : a.order < b.order;
    });
    return result;
}
void Timeline::refresh() {
    horizontalScrollBar()->setRange(
        0, std::max(0, int((double(session_->project().duration()) / Second + 10) * scale_) -
                           viewport()->width() + Header));
    horizontalScrollBar()->setPageStep(std::max(1, viewport()->width() - Header));
    verticalScrollBar()->setRange(
        0, std::max(0, int(tracks().size()) * Row + Ruler - viewport()->height()));
    viewport()->update();
}
void Timeline::resizeEvent(QResizeEvent *e) {
    QAbstractScrollArea::resizeEvent(e);
    refresh();
}
double Timeline::xAt(Time t) const {
    return Header + double(t) / Second * scale_ - horizontalScrollBar()->value();
}
Time Timeline::timeAt(double x) const {
    return std::clamp<Time>(
        Time(std::llround((x - Header + horizontalScrollBar()->value()) / scale_ * Second)), 0,
        MaxTime);
}
int Timeline::rowAt(double y) const {
    return int((y - Ruler + verticalScrollBar()->value()) / Row);
}
QRectF Timeline::rect(const Clip &c) const {
    auto ts = tracks();
    int row = 0;
    for (; row < int(ts.size()); ++row)
        if (ts[row].id == c.trackId)
            break;
    return {xAt(c.start), double(Ruler + row * Row - verticalScrollBar()->value() + 6),
            std::max(2.0, double(c.duration()) / Second * scale_), double(Row - 12)};
}
void Timeline::zoom(double factor) {
    auto t = timeAt(Header + 20);
    scale_ = std::clamp(scale_ * factor, 4.0, 600.0);
    refresh();
    horizontalScrollBar()->setValue(int(double(t) / Second * scale_) - 20);
}
void Timeline::fit() {
    scale_ = std::clamp(
        double(viewport()->width() - Header - 28) /
            (double(std::max<Time>(session_->project().duration(), 5 * Second)) / Second),
        4.0, 600.0);
    horizontalScrollBar()->setValue(0);
    refresh();
}
void Timeline::paintEvent(QPaintEvent *) {
    QPainter p(viewport());
    p.setRenderHint(QPainter::Antialiasing);
    auto bounds = viewport()->rect();
    p.fillRect(bounds, QColor("#11151c"));
    auto ts = tracks();
    p.setFont(QFont(font().family(), 9));
    for (int i = 0; i < int(ts.size()); ++i) {
        int y = Ruler + i * Row - verticalScrollBar()->value();
        p.fillRect(QRect(Header, y, bounds.width() - Header, Row),
                   QColor(i % 2 ? "#151a22" : "#171d26"));
        p.setPen(QColor("#28313d"));
        p.drawLine(0, y + Row, bounds.width(), y + Row);
    }
    p.save();
    p.setClipRect(QRect(Header, Ruler, bounds.width() - Header, bounds.height() - Ruler));
    double step = 1;
    while (step * scale_ < 75)
        step *= 2;
    while (step * scale_ > 220)
        step /= 2;
    Time first = Time(std::floor(double(timeAt(Header)) / Second / step) * step * Second);
    for (Time t = first; xAt(t) < bounds.width(); t += Time(step * Second)) {
        p.setPen(QColor("#232b36"));
        p.drawLine(QPointF(xAt(t), Ruler), QPointF(xAt(t), bounds.height()));
    }
    auto drawClip = [&](const Clip &c, bool ghost) {
        QRectF r = rect(c);
        if (!r.intersects(bounds))
            return;
        auto &t = session_->project().track(c.trackId);
        bool video = t.type == TrackType::Video, selected = c.id == session_->selected;
        QColor fill = video ? QColor("#25594d") : QColor("#304b73");
        if (t.locked)
            fill = QColor("#38404a");
        if (ghost)
            fill.setAlpha(150);
        p.setPen(
            QPen(selected ? QColor("#a7f3d0") : (video ? QColor("#398872") : QColor("#5379ad")),
                 selected ? 2 : 1));
        p.setBrush(fill);
        p.drawRoundedRect(r.adjusted(1, 1, -1, -1), 5, 5);
        p.save();
        p.setClipRect(r.adjusted(7, 3, -7, -3));
        p.setPen(QColor("#e1e8ef"));
        const auto &asset = session_->project().asset(c.assetId);
        auto name =
            QString::fromStdString(asset.kind == AssetKind::Title ? c.title.text : asset.name);
        p.drawText(r.adjusted(11, 4, -10, -20), Qt::AlignVCenter,
                   fontMetrics().elidedText(name, Qt::ElideRight, int(r.width()) - 22));
        p.setPen(QColor("#b2c6cd"));
        p.setFont(QFont(font().family(), 8));
        p.drawText(r.adjusted(11, 24, -10, -3), Qt::AlignVCenter, timecode(c.duration()));
        p.restore();
        p.setFont(QFont(font().family(), 9));
        if (selected) {
            p.setPen(QPen(QColor("#c2ffe6"), 2));
            p.drawLine(r.left() + 5, r.top() + 14, r.left() + 5, r.bottom() - 14);
            p.drawLine(r.right() - 5, r.top() + 14, r.right() - 5, r.bottom() - 14);
        }
    };
    for (const auto &c : session_->project().clips)
        if (!ghost_ || c.id != ghost_->id)
            drawClip(c, false);
    if (ghost_)
        drawClip(*ghost_, true);
    p.restore();
    // Fixed headers stay visible while the time axis scrolls.
    for (int i = 0; i < int(ts.size()); ++i) {
        int y = Ruler + i * Row - verticalScrollBar()->value();
        p.fillRect(QRect(0, y, Header, Row), QColor("#1d232d"));
        p.setPen(QColor("#dbe3ef"));
        p.drawText(QRect(18, y, 45, Row), Qt::AlignVCenter, QString::fromStdString(ts[i].name));
        p.setPen(ts[i].muted ? QColor("#fbbf69") : QColor("#8795a9"));
        p.drawText(QRect(78, y + 17, 23, 25), Qt::AlignCenter, "M");
        p.setPen(ts[i].locked ? QColor("#fbbf69") : QColor("#8795a9"));
        p.drawText(QRect(109, y + 17, 23, 25), Qt::AlignCenter, "L");
    }
    p.fillRect(QRect(0, 0, bounds.width(), Ruler), QColor("#202732"));
    p.setPen(QColor("#93a3b9"));
    p.drawText(QRect(18, 0, Header - 20, Ruler), Qt::AlignVCenter, tr("TRACKS"));
    p.save();
    p.setClipRect(QRect(Header, 0, bounds.width() - Header, bounds.height()));
    for (Time t = first; xAt(t) < bounds.width(); t += Time(step * Second)) {
        p.setPen(QColor("#93a3b9"));
        p.drawText(QPointF(xAt(t) + 7, 23), timecode(t).left(8));
        p.setPen(QColor("#485261"));
        p.drawLine(QPointF(xAt(t), 30), QPointF(xAt(t), 38));
    }
    double head = xAt(session_->playhead);
    p.setPen(QPen(QColor("#f5bc78"), 1.5));
    p.drawLine(QPointF(head, 0), QPointF(head, bounds.height()));
    p.setBrush(QColor("#f5bc78"));
    p.drawPolygon(
        QPolygonF{{head - 5, 0}, {head + 5, 0}, {head + 5, 9}, {head, 15}, {head - 5, 9}});
    p.restore();
}
Time Timeline::snapped(Time t, const std::string &excluded) const {
    t = snapToFrame(std::max<Time>(0, t), session_->project().settings);
    if (!snapping_)
        return t;
    Time best = t, threshold = Time(8 / scale_ * Second), distance = threshold;
    auto consider = [&](Time point) {
        auto d = std::abs(point - t);
        if (d < distance) {
            distance = d;
            best = point;
        }
    };
    consider(session_->playhead);
    consider(0);
    for (const auto &c : session_->project().clips)
        if (c.id != excluded) {
            consider(c.start);
            consider(c.end());
        }
    return best;
}
void Timeline::mousePressEvent(QMouseEvent *e) {
    if (e->button() != Qt::LeftButton)
        return;
    setFocus();
    auto pos = e->position();
    press_ = pos;
    if (pos.x() < Header) {
        int row = rowAt(pos.y());
        auto ts = tracks();
        if (pos.y() > Ruler && row >= 0 && row < int(ts.size())) {
            const auto t = ts[row];
            if (pos.x() >= 75 && pos.x() < 105)
                session_->edit([&](Editor &ed) { ed.trackState(t.id, !t.muted, t.locked); });
            else if (pos.x() >= 105)
                session_->edit([&](Editor &ed) { ed.trackState(t.id, t.muted, !t.locked); });
        }
        return;
    }
    if (pos.y() >= Ruler)
        for (const auto &c : session_->project().clips)
            if (rect(c).contains(pos)) {
                session_->select(c.id);
                if (session_->project().track(c.trackId).locked)
                    return;
                original_ = c;
                ghost_ = c;
                auto r = rect(c);
                mode_ = pos.x() - r.left() < 8 ? 1 : r.right() - pos.x() < 8 ? 2 : 0;
                return;
            }
    if (pos.y() >= Ruler)
        session_->select({});
    scrubbing_ = true;
    session_->seek(snapToFrame(timeAt(pos.x()), session_->project().settings));
}
void Timeline::mouseMoveEvent(QMouseEvent *e) {
    auto pos = e->position();
    if (scrubbing_) {
        session_->seek(snapToFrame(timeAt(pos.x()), session_->project().settings));
        return;
    }
    if (!original_) {
        bool edge = false;
        for (const auto &c : session_->project().clips)
            if (rect(c).contains(pos)) {
                auto r = rect(c);
                edge = pos.x() - r.left() < 8 || r.right() - pos.x() < 8;
            }
        viewport()->setCursor(edge ? Qt::SizeHorCursor : Qt::ArrowCursor);
        return;
    }
    ghost_ = *original_;
    auto &c = *ghost_;
    Time delta = Time(std::llround((pos.x() - press_.x()) / scale_ * Second));
    const Time min = frameTime(1, session_->project().settings);
    if (mode_ == 0) {
        c.start = snapped(c.start + delta, c.id);
        auto ts = tracks();
        int row = rowAt(pos.y());
        if (row >= 0 && row < int(ts.size()) &&
            ts[row].type == session_->project().track(c.trackId).type)
            c.trackId = ts[row].id;
    } else if (mode_ == 1) {
        Time start = snapped(c.start + delta, c.id);
        Time change = std::clamp(start - c.start, -std::min(c.start, c.sourceIn),
                                 std::max<Time>(0, c.duration() - min));
        c.start += change;
        c.sourceIn += change;
    } else {
        Time end = snapped(c.end() + delta, c.id);
        c.sourceOut =
            std::clamp(c.sourceOut + end - c.end(), std::min(c.sourceIn + min, c.sourceOut),
                       session_->project().asset(c.assetId).duration);
    }
    viewport()->update();
}
void Timeline::mouseReleaseEvent(QMouseEvent *) {
    scrubbing_ = false;
    if (!ghost_)
        return;
    auto c = *ghost_, old = *original_;
    ghost_.reset();
    original_.reset();
    if (c != old)
        session_->edit([&](Editor &ed) {
            if (mode_ == 0)
                ed.move(c.id, c.trackId, c.start);
            else
                ed.trim(c.id, c.sourceIn, c.sourceOut, c.start);
        });
    viewport()->update();
}
void Timeline::wheelEvent(QWheelEvent *e) {
    if (e->modifiers() & Qt::ControlModifier) {
        zoom(e->angleDelta().y() > 0 ? 1.2 : 1 / 1.2);
        e->accept();
    } else if (e->modifiers() & Qt::ShiftModifier) {
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - e->angleDelta().y());
        e->accept();
    } else
        QAbstractScrollArea::wheelEvent(e);
}
void Timeline::dragEnterEvent(QDragEnterEvent *e) {
    if (e->mimeData()->hasFormat("application/x-perplex-asset") || e->mimeData()->hasUrls())
        e->acceptProposedAction();
}
void Timeline::dragMoveEvent(QDragMoveEvent *e) {
    e->acceptProposedAction();
}
void Timeline::dropEvent(QDropEvent *e) {
    if (e->mimeData()->hasUrls()) {
        QStringList files;
        for (auto u : e->mimeData()->urls())
            if (u.isLocalFile())
                files << u.toLocalFile();
        emit filesDropped(files);
        e->acceptProposedAction();
        return;
    }
    int row = rowAt(e->position().y());
    auto ts = tracks();
    if (row < 0 || row >= int(ts.size()))
        return;
    auto asset = e->mimeData()->data("application/x-perplex-asset").toStdString();
    auto track = ts[row].id;
    auto start = snapped(timeAt(e->position().x()), {});
    session_->edit([&](Editor &ed) { session_->selected = ed.addClip(asset, track, start); });
    e->acceptProposedAction();
}
void Timeline::contextMenuEvent(QContextMenuEvent *e) {
    auto ts = tracks();
    int row = rowAt(e->pos().y());
    if (row < 0 || row >= int(ts.size()))
        return;
    const auto t = ts[row];
    QMenu menu(this);
    auto mute = menu.addAction(tr("Mute track audio"));
    mute->setCheckable(true);
    mute->setChecked(t.muted);
    mute->setEnabled(!t.locked);
    auto lock = menu.addAction(tr("Lock track"));
    lock->setCheckable(true);
    lock->setChecked(t.locked);
    auto a = menu.exec(e->globalPos());
    if (a == mute)
        session_->edit([&](Editor &ed) { ed.trackState(t.id, !t.muted, t.locked); });
    if (a == lock)
        session_->edit([&](Editor &ed) { ed.trackState(t.id, t.muted, !t.locked); });
}
} // namespace pms
