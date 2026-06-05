#pragma once
#include "Player.h"
#include "SeekSlider.h"
#include "VolumeSlider.h"
#include "TrackInfoWidget.h"
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace UIXPlayer {

class PlayerControls : public QWidget {
    Q_OBJECT
public:
    explicit PlayerControls(Player& player, QWidget* parent = nullptr);

    void updateState     (QMediaPlayer::PlaybackState s);
    void updatePosition  (qint64 posMs);
    void updateDuration  (qint64 durMs);
    void updateTrack     (const Track* t);
    void updateVolume    (float v);
    void updateShuffle   (ShuffleMode m);
    void updateRepeat    (RepeatMode  m);

signals:
    void playClicked    ();
    void pauseClicked   ();
    void stopClicked    ();
    void nextClicked    ();
    void prevClicked    ();
    void seekRequested  (qint64 ms);
    void volumeChanged  (float v);
    void muteToggled    (bool  m);
    void shuffleClicked ();
    void repeatClicked  ();

private:
    Player&           player_;
    TrackInfoWidget*  trackInfo_;
    SeekSlider*       seekSlider_;
    VolumeSlider*     volumeSlider_;

    QPushButton* prevBtn_;
    QPushButton* playBtn_;
    QPushButton* pauseBtn_;
    QPushButton* stopBtn_;
    QPushButton* nextBtn_;
    QPushButton* shuffleBtn_;
    QPushButton* repeatBtn_;

    QLabel* timeElapsed_;
    QLabel* timeDuration_;

    qint64 duration_ms_ = 0;

    void setupUI();
    void connectSignals();
    static QString msToStr(qint64 ms);
};

} /* namespace UIXPlayer */
