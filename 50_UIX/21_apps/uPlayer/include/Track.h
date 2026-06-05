#pragma once
#include <QString>
#include <QUrl>
#include <QImage>
#include <cstdint>

namespace UIXPlayer {

struct Track {
    uint32_t id           = 0;
    QString  title;
    QString  artist;
    QString  album;
    QString  genre;
    QUrl     url;            /* file:// or http:// stream         */
    int64_t  duration_ms  = 0;
    uint32_t sample_rate  = 44100;
    uint32_t bit_rate     = 0;
    QString  format;         /* "MP3","FLAC","WAV","OGG","AAC"   */
    QImage   cover_art;
    int      track_number = 0;
    int      year         = 0;

    Track() = default;
    Track(uint32_t id, const QString& title,
          const QString& artist, const QString& album,
          const QUrl& url, int64_t dur_ms,
          const QString& fmt = "MP3")
        : id(id), title(title), artist(artist),
          album(album), url(url),
          duration_ms(dur_ms), format(fmt) {}

    QString durationStr() const;
    bool    isValid()     const { return !url.isEmpty(); }
    bool operator==(const Track& o) const { return id == o.id; }
};

} /* namespace UIXPlayer */
