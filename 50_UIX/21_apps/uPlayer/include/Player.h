#pragma once
#include "Playlist.h"
#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <memory>

namespace UIXPlayer {

class Player : public QObject {
    Q_OBJECT
    Q_PROPERTY(float volume READ volume WRITE setVolume
               NOTIFY volumeChanged)

public:
    explicit Player(QObject* parent = nullptr);
    ~Player() override = default;

    /* playback */
    void play    ();
    void pause   ();
    void stop    ();
    void nextTrack();
    void prevTrack();
    void seek    (qint64 posMs);

    /* volume 0.0 – 1.0 */
    float volume () const;
    void  setVolume(float v);
    void  setMuted (bool m);
    bool  isMuted  () const;

    /* state */
    QMediaPlayer::PlaybackState state() const;
    qint64 position () const;
    qint64 duration () const;

    /* playlist access */
    Playlist&       playlist()       { return *playlist_; }
    const Playlist& playlist() const { return *playlist_; }

    /* mode helpers */
    void toggleShuffle();
    void cycleRepeat  ();

    /* direct media player access for video widget etc. */
    QMediaPlayer* mediaPlayer() { return player_.get(); }

signals:
    void stateChanged      (QMediaPlayer::PlaybackState);
    void positionChanged   (qint64 posMs);
    void durationChanged   (qint64 durMs);
    void trackChanged      (const Track* t);
    void volumeChanged     (float v);
    void errorOccurred     (const QString& msg);
    void shuffleModeChanged(ShuffleMode m);
    void repeatModeChanged (RepeatMode  m);

private slots:
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onPlayerError(QMediaPlayer::Error err, const QString& msg);

private:
    std::unique_ptr<QMediaPlayer> player_;
    std::unique_ptr<QAudioOutput> audioOut_;
    std::unique_ptr<Playlist>     playlist_;

    void loadCurrentTrack();
};

} /* namespace UIXPlayer */
