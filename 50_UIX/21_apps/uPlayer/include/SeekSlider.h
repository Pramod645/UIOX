#pragma once
#include <QSlider>

namespace UIXPlayer {

/*
 * SeekSlider — a QSlider that:
 *  - responds to single click (seek to clicked position)
 *  - supports touch gestures (swipe left/right = seek ±10s)
 *  - shows hover tooltip with time
 */
class SeekSlider : public QSlider {
    Q_OBJECT
public:
    explicit SeekSlider(Qt::Orientation o = Qt::Horizontal,
                         QWidget* parent = nullptr);

    void setDuration(qint64 durationMs);

signals:
    void seekRequested(qint64 posMs);

protected:
    void mousePressEvent  (QMouseEvent*  e) override;
    void mouseMoveEvent   (QMouseEvent*  e) override;
    void mouseReleaseEvent(QMouseEvent*  e) override;
    void touchEvent       (QTouchEvent*  e) override;
    void paintEvent       (QPaintEvent*  e) override;
    bool event            (QEvent*       e) override;

private:
    qint64 duration_ms_  = 0;
    bool   dragging_     = false;
    int    hoverX_       = -1;

    qint64 xToMs(int x) const;
    QString msToStr(qint64 ms) const;
};

} /* namespace UIXPlayer */
