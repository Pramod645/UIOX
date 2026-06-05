#include "VisualizerWidget.h"

namespace UIXPlayer {

VisualizerWidget::VisualizerWidget(QWidget* parent)
    : QWidget(parent)
    , rng_(std::random_device{}())
{
    bars_   .resize(BAR_COUNT, 0.0f);
    targets_.resize(BAR_COUNT, 0.0f);
    peaks_  .resize(BAR_COUNT, 0.0f);

    timer_.setInterval(1000 / FPS);
    connect(&timer_, &QTimer::timeout,
            this, &VisualizerWidget::updateBars);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(60);
    setMaximumHeight(80);
}

void VisualizerWidget::start()
{
    playing_ = true;
    timer_.start();
}

void VisualizerWidget::stop()
{
    playing_ = false;
    std::fill(bars_.begin(),    bars_.end(),    0.0f);
    std::fill(peaks_.begin(),   peaks_.end(),   0.0f);
    std::fill(targets_.begin(), targets_.end(), 0.0f);
    update();
}

void VisualizerWidget::pause()
{
    playing_ = false;
}

void VisualizerWidget::updateBars()
{
    std::uniform_real_distribution<float> dist(0.1f, 1.0f);
    for (int i = 0; i < BAR_COUNT; ++i) {
        if (playing_)
            targets_[i] = dist(rng_);
        else
            targets_[i] *= 0.85f;

        /* smooth interpolation */
        float diff = targets_[i] - bars_[i];
        bars_[i] += diff * (diff > 0 ? 0.3f : 0.15f);

        /* peak hold */
        if (bars_[i] >= peaks_[i])
            peaks_[i] = bars_[i];
        else
            peaks_[i] -= 0.015f;

        peaks_[i] = std::clamp(peaks_[i], 0.0f, 1.0f);
    }
    update();
}

void VisualizerWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int barW = w / BAR_COUNT;
    int gap  = 2;

    for (int i = 0; i < BAR_COUNT; ++i) {
        float t    = (float)i / BAR_COUNT;
        int   barH = (int)(bars_[i] * h);
        int   x    = i * barW;
        int   y    = h - barH;

        /* gradient: red → orange → yellow */
        QColor col;
        if (bars_[i] > 0.8f)       col = QColor(255, 80,  80);
        else if (bars_[i] > 0.5f)  col = QColor(233, 69, 96);
        else                        col = QColor(150, 50, 200);

        (void)t;
        p.fillRect(x + gap/2, y, barW - gap, barH, col);

        /* peak marker */
        int peakY = h - (int)(peaks_[i] * h);
        p.setPen(QPen(QColor(255, 255, 255, 120), 1));
        p.drawLine(x + gap/2, peakY, x + barW - gap/2, peakY);
    }
}

void VisualizerWidget::resizeEvent(QResizeEvent*)
{
    update();
}

} /* namespace UIXPlayer */
