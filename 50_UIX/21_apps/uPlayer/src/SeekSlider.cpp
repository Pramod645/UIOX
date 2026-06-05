#include "SeekSlider.h"
#include <QMouseEvent>
#include <QTouchEvent>
#include <QPainter>
#include <QTime>
#include <QToolTip>

namespace UIXPlayer {

SeekSlider::SeekSlider(Qt::Orientation o, QWidget* parent)
    : QSlider(o, parent)
{
    setObjectName("seekSlider");
    setAttribute(Qt::WA_AcceptTouchEvents);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(20);
}

void SeekSlider::setDuration(qint64 ms)
{
    duration_ms_ = ms;
    setMaximum(ms > 0 ? (int)(ms / 1000) : 0);
}

qint64 SeekSlider::xToMs(int x) const
{
    if (width() <= 0 || duration_ms_ <= 0) return 0;
    double ratio = (double)x / width();
    ratio = std::clamp(ratio, 0.0, 1.0);
    return (qint64)(ratio * duration_ms_);
}

QString SeekSlider::msToStr(qint64 ms) const
{
    int secs = (int)(ms / 1000);
    QTime t(0, secs / 60, secs % 60);
    return t.toString("m:ss");
}

void SeekSlider::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        dragging_ = true;
        qint64 ms = xToMs(e->pos().x());
        emit seekRequested(ms);
    }
    QSlider::mousePressEvent(e);
}

void SeekSlider::mouseMoveEvent(QMouseEvent* e)
{
    hoverX_ = e->pos().x();
    if (dragging_) {
        qint64 ms = xToMs(e->pos().x());
        QToolTip::showText(e->globalPosition().toPoint(),
                           msToStr(ms), this);
        emit seekRequested(ms);
    } else {
        qint64 ms = xToMs(e->pos().x());
        QToolTip::showText(e->globalPosition().toPoint(),
                           msToStr(ms), this);
    }
    QSlider::mouseMoveEvent(e);
    update();
}

void SeekSlider::mouseReleaseEvent(QMouseEvent* e)
{
    dragging_ = false;
    QSlider::mouseReleaseEvent(e);
}

bool SeekSlider::event(QEvent* e)
{
    if (e->type() == QEvent::TouchBegin ||
        e->type() == QEvent::TouchUpdate ||
        e->type() == QEvent::TouchEnd) {
        QTouchEvent* te = static_cast<QTouchEvent*>(e);
        if (!te->points().isEmpty()) {
            QPointF pos = te->points().first().position();
            qint64 ms = xToMs((int)pos.x());
            emit seekRequested(ms);
            e->accept();
            return true;
        }
    }
    return QSlider::event(e);
}

void SeekSlider::paintEvent(QPaintEvent* e)
{
    QSlider::paintEvent(e);

    /* hover time marker */
    if (hoverX_ >= 0 && !dragging_) {
        QPainter p(this);
        p.setPen(QPen(QColor("#e94560"), 2));
        p.drawLine(hoverX_, 0, hoverX_, height());
    }
}

void SeekSlider::touchEvent(QTouchEvent* e) { event(e); }

} /* namespace UIXPlayer */
