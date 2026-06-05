#pragma once
#include <QWidget>
#include <QTimer>
#include <QVector>
#include <QPainter>
#include <cmath>
#include <random>

namespace UIXPlayer {

/* Simple animated bar visualizer (decorative) */
class VisualizerWidget : public QWidget {
    Q_OBJECT
public:
    explicit VisualizerWidget(QWidget* parent = nullptr);

    void start();
    void stop ();
    void pause();

protected:
    void paintEvent(QPaintEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;

private:
    QTimer         timer_;
    QVector<float> bars_;
    QVector<float> targets_;
    QVector<float> peaks_;
    bool           playing_ = false;
    std::mt19937   rng_;

    static constexpr int  BAR_COUNT   = 32;
    static constexpr int  FPS         = 30;

    void updateBars();
};

} /* namespace UIXPlayer */
