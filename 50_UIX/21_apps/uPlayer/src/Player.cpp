#include "Player.h"

namespace UIXPlayer {

Player::Player(QObject* parent)
    : QObject(parent)
{
    player_   = std::make_unique<QMediaPlayer>(this);
    audioOut_ = std::make_unique<QAudioOutput>(this);
    playlist_ = std::make_unique<Playlist>("My Playlist", this);

    player_->setAudioOutput(audioOut_.get());
    audioOut_->setVolume(0.8f);

    connect(player_.get(), &QMediaPlayer::playbackStateChanged,
            this, &Player::stateChanged);
    connect(player_.get(), &QMediaPlayer::positionChanged,
            this, &Player::positionChanged);
    connect(player_.get(), &QMediaPlayer::durationChanged,
            this, &Player::durationChanged);
    connect(player_.get(), &QMediaPlayer::mediaStatusChanged,
            this, &Player::onMediaStatusChanged);
    connect(player_.get(), &QMediaPlayer::errorOccurred,
            this, &Player::onPlayerError);
}

void Player::play()
{
    if (player_->playbackState() == QMediaPlayer::PausedState) {
        player_->play();
        return;
    }
    loadCurrentTrack();
    player_->play();
}

void Player::pause()
{
    if (player_->playbackState() == QMediaPlayer::PlayingState)
        player_->pause();
    else if (player_->playbackState() == QMediaPlayer::PausedState)
        player_->play();
}

void Player::stop()  { player_->stop(); }

void Player::nextTrack()
{
    const Track* t = playlist_->next();
    if (t) { loadCurrentTrack(); player_->play(); }
    emit trackChanged(t);
}

void Player::prevTrack()
{
    if (player_->position() > 3000) {
        player_->setPosition(0);
        return;
    }
    const Track* t = playlist_->previous();
    if (t) { loadCurrentTrack(); player_->play(); }
    emit trackChanged(t);
}

void Player::seek(qint64 posMs)  { player_->setPosition(posMs); }

float Player::volume() const     { return audioOut_->volume(); }

void Player::setVolume(float v)
{
    float clamped = std::clamp(v, 0.0f, 1.0f);
    audioOut_->setVolume(clamped);
    emit volumeChanged(clamped);
}

void Player::setMuted(bool m)    { audioOut_->setMuted(m); }
bool Player::isMuted() const     { return audioOut_->isMuted(); }

QMediaPlayer::PlaybackState Player::state() const
{ return player_->playbackState(); }

qint64 Player::position() const  { return player_->position(); }
qint64 Player::duration() const  { return player_->duration(); }

void Player::toggleShuffle()
{
    ShuffleMode m = (playlist_->shuffleMode() == ShuffleMode::Off)
                  ? ShuffleMode::On : ShuffleMode::Off;
    playlist_->setShuffleMode(m);
    emit shuffleModeChanged(m);
}

void Player::cycleRepeat()
{
    RepeatMode next;
    switch (playlist_->repeatMode()) {
        case RepeatMode::Off: next = RepeatMode::All; break;
        case RepeatMode::All: next = RepeatMode::One; break;
        default:              next = RepeatMode::Off; break;
    }
    playlist_->setRepeatMode(next);
    emit repeatModeChanged(next);
}

void Player::loadCurrentTrack()
{
    const Track* t = playlist_->current();
    if (!t) return;
    player_->setSource(t->url);
    emit trackChanged(t);
}

void Player::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        const Track* t = playlist_->next();
        if (t) { loadCurrentTrack(); player_->play(); }
        else   { player_->stop(); }
        emit trackChanged(t);
    }
}

void Player::onPlayerError(QMediaPlayer::Error /*err*/,
                             const QString& msg)
{
    emit errorOccurred(msg);
}

} /* namespace UIXPlayer */
