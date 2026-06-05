#pragma once
#include "Track.h"
#include <QWidget>
#include <QLabel>
#include <QScrollingLabel>  /* custom — defined below */

namespace UIXPlayer {

class TrackInfoWidget : public QWidget {
    Q_OBJECT
public:
    explicit TrackInfoWidget(QWidget* parent = nullptr);
    void setTrack(const Track* t);
    void clear   ();

private:
    QLabel* coverLabel_;
    QLabel* titleLabel_;
    QLabel* artistLabel_;
    QLabel* albumLabel_;
    QLabel* formatLabel_;

    void setupUI();
};

} /* namespace UIXPlayer */
