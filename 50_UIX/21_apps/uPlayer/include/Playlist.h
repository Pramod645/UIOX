#pragma once
#include "Track.h"
#include <QObject>
#include <QVector>
#include <QString>

namespace UIXPlayer {

enum class ShuffleMode { Off, On };
enum class RepeatMode  { Off, One, All };

class Playlist : public QObject {
    Q_OBJECT
public:
    explicit Playlist(const QString& name = "Playlist",
                      QObject* parent = nullptr);

    /* track management */
    void    addTrack      (const Track& t);
    void    insertTrack   (int idx, const Track& t);
    void    removeTrack   (int idx);
    void    clear         ();
    int     count         () const { return tracks_.size(); }
    bool    isEmpty       () const { return tracks_.isEmpty(); }

    const Track* trackAt    (int idx) const;
    const Track* current    () const;
    int          currentIdx () const { return currentIdx_; }

    /* navigation — returns nullptr at end (no repeat) */
    const Track* next     ();
    const Track* previous ();
    bool         seekTo   (int idx);

    /* modes */
    void        setShuffleMode(ShuffleMode m);
    void        setRepeatMode (RepeatMode  m);
    ShuffleMode shuffleMode   () const { return shuffle_; }
    RepeatMode  repeatMode    () const { return repeat_;  }

    const QString&        name  () const { return name_; }
    const QVector<Track>& tracks() const { return tracks_; }

    /* search */
    QVector<int> search(const QString& query) const;

    /* persistence */
    bool saveM3U(const QString& path) const;
    bool loadM3U(const QString& path);

signals:
    void trackAdded   (int idx);
    void trackRemoved (int idx);
    void currentChanged(int idx);
    void playlistCleared();

private:
    QString        name_;
    QVector<Track> tracks_;
    QVector<int>   shuffleOrder_;
    int            currentIdx_ = 0;
    ShuffleMode    shuffle_    = ShuffleMode::Off;
    RepeatMode     repeat_     = RepeatMode::Off;
    uint32_t       nextId_     = 1;

    void rebuildShuffle();
    int  resolveIdx(int raw) const;
};

} /* namespace UIXPlayer */
