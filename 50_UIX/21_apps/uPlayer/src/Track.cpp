#include "Track.h"
#include <QTime>

namespace UIXPlayer {

QString Track::durationStr() const
{
    qint64 secs = duration_ms / 1000;
    QTime t(0, (int)(secs / 60), (int)(secs % 60));
    return secs >= 3600 ? t.toString("h:mm:ss")
                        : t.toString("m:ss");
}

} /* namespace UIXPlayer */
