#include "PlayerControls.h"
#include <QTime>

namespace UIXPlayer {

PlayerControls::PlayerControls(Player& player, QWidget* parent)
    : QWidget(parent), player_(player)
{
    setObjectName("controlsBar");
    setupUI();
    connectSignals();
}

QString PlayerControls::msToStr(qint64 ms)
{
    int secs = (int)(ms / 1000);
    QTime t(0, secs / 60, secs % 60);
    return secs >= 3600 ? t.toString("h:mm:ss")
                        : t.toString("m:ss");
}

void PlayerControls::setupUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 8, 12, 8);
    root->setSpacing(6);

    /* ── Track info ───────────────────────────────────────── */
    trackInfo_ = new TrackInfoWidget(this);
    root->addWidget(trackInfo_);

    /* ── Seek row ─────────────────────────────────────────── */
    auto* seekRow = new QHBoxLayout;
    seekRow->setSpacing(8);

    timeElapsed_ = new QLabel("0:00", this);
    timeElapsed_->setObjectName("timeElapsed");
    timeElapsed_->setFixedWidth(40);

    seekSlider_ = new SeekSlider(Qt::Horizontal, this);

    timeDuration_ = new QLabel("0:00", this);
    timeDuration_->setObjectName("timeDuration");
    timeDuration_->setFixedWidth(40);
    timeDuration_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    seekRow->addWidget(timeElapsed_);
    seekRow->addWidget(seekSlider_, 1);
    seekRow->addWidget(timeDuration_);
    root->addLayout(seekRow);

    /* ── Button row ───────────────────────────────────────── */
    auto* btnRow = new QHBoxLayout;
    btnRow->setSpacing(10);
    btnRow->setAlignment(Qt::AlignCenter);

    shuffleBtn_ = new QPushButton("⇄", this);
    shuffleBtn_->setObjectName("shuffleBtn");
    shuffleBtn_->setCheckable(true);
    shuffleBtn_->setToolTip("Shuffle (X)");
    shuffleBtn_->setFixedSize(34, 34);

    prevBtn_ = new QPushButton("⏮", this);
    prevBtn_->setObjectName("prevBtn");
    prevBtn_->setToolTip("Previous (Left)");
    prevBtn_->setFixedSize(44, 44);

    playBtn_ = new QPushButton("▶", this);
    playBtn_->setObjectName("playBtn");
    playBtn_->setToolTip("Play (Space)");
    playBtn_->setFixedSize(52, 52);

    pauseBtn_ = new QPushButton("⏸", this);
    pauseBtn_->setObjectName("pauseBtn");
    pauseBtn_->setToolTip("Pause (Space)");
    pauseBtn_->setFixedSize(44, 44);
    pauseBtn_->hide();

    stopBtn_ = new QPushButton("⏹", this);
    stopBtn_->setObjectName("stopBtn");
    stopBtn_->setToolTip("Stop (S)");
    stopBtn_->setFixedSize(44, 44);

    nextBtn_ = new QPushButton("⏭", this);
    nextBtn_->setObjectName("nextBtn");
    nextBtn_->setToolTip("Next (Right)");
    nextBtn_->setFixedSize(44, 44);

    repeatBtn_ = new QPushButton("↻", this);
    repeatBtn_->setObjectName("repeatBtn");
    repeatBtn_->setCheckable(true);
    repeatBtn_->setToolTip("Repeat (R)");
    repeatBtn_->setFixedSize(34, 34);

    volumeSlider_ = new VolumeSlider(this);

    btnRow->addWidget(shuffleBtn_);
    btnRow->addStretch(1);
    btnRow->addWidget(prevBtn_);
    btnRow->addWidget(playBtn_);
    btnRow->addWidget(pauseBtn_);
    btnRow->addWidget(stopBtn_);
    btnRow->addWidget(nextBtn_);
    btnRow->addStretch(1);
    btnRow->addWidget(repeatBtn_);
    btnRow->addSpacing(20);
    btnRow->addWidget(volumeSlider_);

    root->addLayout(btnRow);
}

void PlayerControls::connectSignals()
{
    connect(playBtn_,  &QPushButton::clicked,
            this, &PlayerControls::playClicked);
    connect(pauseBtn_, &QPushButton::clicked,
            this, &PlayerControls::pauseClicked);
    connect(stopBtn_,  &QPushButton::clicked,
            this, &PlayerControls::stopClicked);
    connect(nextBtn_,  &QPushButton::clicked,
            this, &PlayerControls::nextClicked);
    connect(prevBtn_,  &QPushButton::clicked,
            this, &PlayerControls::prevClicked);

    connect(seekSlider_, &SeekSlider::seekRequested,
            this, &PlayerControls::seekRequested);

    connect(volumeSlider_, &VolumeSlider::volumeChanged,
            this, &PlayerControls::volumeChanged);
    connect(volumeSlider_, &VolumeSlider::muteToggled,
            this, &PlayerControls::muteToggled);

    connect(shuffleBtn_, &QPushButton::clicked,
            this, &PlayerControls::shuffleClicked);
    connect(repeatBtn_,  &QPushButton::clicked,
            this, &PlayerControls::repeatClicked);
}

void PlayerControls::updateState(QMediaPlayer::PlaybackState s)
{
    bool playing = (s == QMediaPlayer::PlayingState);
    playBtn_ ->setVisible(!playing);
    pauseBtn_->setVisible(playing);
}

void PlayerControls::updatePosition(qint64 posMs)
{
    timeElapsed_->setText(msToStr(posMs));
    if (duration_ms_ > 0)
        seekSlider_->setValue((int)(posMs / 1000));
}

void PlayerControls::updateDuration(qint64 durMs)
{
    duration_ms_ = durMs;
    seekSlider_->setDuration(durMs);
    timeDuration_->setText(msToStr(durMs));
}

void PlayerControls::updateTrack(const Track* t)
{
    trackInfo_->setTrack(t);
}

void PlayerControls::updateVolume(float v)
{
    volumeSlider_->setVolume(v);
}

void PlayerControls::updateShuffle(ShuffleMode m)
{
    shuffleBtn_->setChecked(m == ShuffleMode::On);
    shuffleBtn_->setToolTip(
        m == ShuffleMode::On ? "Shuffle: ON" : "Shuffle: OFF");
}

void PlayerControls::updateRepeat(RepeatMode m)
{
    bool on = (m != RepeatMode::Off);
    repeatBtn_->setChecked(on);
    switch (m) {
        case RepeatMode::Off: repeatBtn_->setText("↻");
                               repeatBtn_->setToolTip("Repeat: OFF"); break;
        case RepeatMode::All: repeatBtn_->setText("↻");
                               repeatBtn_->setToolTip("Repeat: ALL"); break;
        case RepeatMode::One: repeatBtn_->setText("↺");
                               repeatBtn_->setToolTip("Repeat: ONE"); break;
    }
}

} /* namespace UIXPlayer */
