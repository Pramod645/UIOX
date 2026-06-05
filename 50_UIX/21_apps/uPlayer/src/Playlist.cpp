#include "Playlist.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QRandomGenerator>
#include <algorithm>

namespace UIXPlayer {

Playlist::Playlist(const QString& name, QObject* parent)
    : QObject(parent), name_(name)
{}

void Playlist::addTrack(const Track& t)
{
    Track copy  = t;
    copy.id     = nextId_++;
    int idx     = tracks_.size();
    tracks_.append(copy);
    rebuildShuffle();
    emit trackAdded(idx);
}

void Playlist::insertTrack(int idx, const Track& t)
{
    Track copy = t;
    copy.id    = nextId_++;
    tracks_.insert(idx, copy);
    rebuildShuffle();
    emit trackAdded(idx);
}

void Playlist::removeTrack(int idx)
{
    if (idx < 0 || idx >= tracks_.size()) return;
    tracks_.remove(idx);
    if (currentIdx_ >= tracks_.size() && !tracks_.isEmpty())
        currentIdx_ = tracks_.size() - 1;
    rebuildShuffle();
    emit trackRemoved(idx);
}

void Playlist::clear()
{
    tracks_.clear();
    shuffleOrder_.clear();
    currentIdx_ = 0;
    emit playlistCleared();
}

const Track* Playlist::trackAt(int idx) const
{
    if (idx < 0 || idx >= tracks_.size()) return nullptr;
    return &tracks_[idx];
}

const Track* Playlist::current() const
{
    if (tracks_.isEmpty()) return nullptr;
    return &tracks_[resolveIdx(currentIdx_)];
}

const Track* Playlist::next()
{
    if (tracks_.isEmpty()) return nullptr;
    if (repeat_ == RepeatMode::One) return current();

    if (currentIdx_ + 1 < tracks_.size()) {
        ++currentIdx_;
    } else if (repeat_ == RepeatMode::All) {
        currentIdx_ = 0;
        if (shuffle_ == ShuffleMode::On)
            rebuildShuffle();
    } else {
        return nullptr;
    }
    emit currentChanged(currentIdx_);
    return current();
}

const Track* Playlist::previous()
{
    if (tracks_.isEmpty()) return nullptr;
    if (currentIdx_ > 0) {
        --currentIdx_;
    } else if (repeat_ == RepeatMode::All) {
        currentIdx_ = tracks_.size() - 1;
    }
    emit currentChanged(currentIdx_);
    return current();
}

bool Playlist::seekTo(int idx)
{
    if (idx < 0 || idx >= tracks_.size()) return false;
    currentIdx_ = idx;
    emit currentChanged(idx);
    return true;
}

void Playlist::setShuffleMode(ShuffleMode m)
{
    shuffle_ = m;
    rebuildShuffle();
}

void Playlist::setRepeatMode(RepeatMode m) { repeat_ = m; }

void Playlist::rebuildShuffle()
{
    shuffleOrder_.resize(tracks_.size());
    for (int i = 0; i < tracks_.size(); ++i)
        shuffleOrder_[i] = i;
    if (shuffle_ == ShuffleMode::On) {
        for (int i = shuffleOrder_.size() - 1; i > 0; --i) {
            int j = (int)(QRandomGenerator::global()->bounded(i + 1));
            std::swap(shuffleOrder_[i], shuffleOrder_[j]);
        }
    }
}

int Playlist::resolveIdx(int raw) const
{
    if (shuffle_ == ShuffleMode::On &&
        raw < shuffleOrder_.size())
        return shuffleOrder_[raw];
    return raw;
}

QVector<int> Playlist::search(const QString& query) const
{
    QString q = query.toLower();
    QVector<int> results;
    for (int i = 0; i < tracks_.size(); ++i) {
        const Track& t = tracks_[i];
        if (t.title .toLower().contains(q) ||
            t.artist.toLower().contains(q) ||
            t.album .toLower().contains(q))
            results.append(i);
    }
    return results;
}

bool Playlist::saveM3U(const QString& path) const
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&f);
    out << "#EXTM3U\n";
    out << "#PLAYLIST:" << name_ << "\n";
    for (const auto& t : tracks_) {
        out << "#EXTINF:" << (t.duration_ms / 1000)
            << "," << t.artist << " - " << t.title << "\n";
        out << t.url.toString() << "\n";
    }
    return true;
}

bool Playlist::loadM3U(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    clear();
    QTextStream in(&f);
    Track current_track;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith("#PLAYLIST:")) {
            name_ = line.mid(10);
        } else if (line.startsWith("#EXTINF:")) {
            QString info = line.mid(8);
            int comma = info.indexOf(',');
            if (comma >= 0) {
                current_track.duration_ms =
                    info.left(comma).toLongLong() * 1000;
                QString meta = info.mid(comma + 1);
                int dash = meta.indexOf(" - ");
                if (dash >= 0) {
                    current_track.artist = meta.left(dash).trimmed();
                    current_track.title  = meta.mid(dash + 3).trimmed();
                } else {
                    current_track.title = meta.trimmed();
                }
            }
        } else if (!line.startsWith('#') && !line.isEmpty()) {
            current_track.url = QUrl(line);
            QFileInfo fi(line);
            if (current_track.title.isEmpty())
                current_track.title = fi.baseName();
            current_track.format = fi.suffix().toUpper();
            addTrack(current_track);
            current_track = Track();
        }
    }
    return true;
}

} /* namespace UIXPlayer */
