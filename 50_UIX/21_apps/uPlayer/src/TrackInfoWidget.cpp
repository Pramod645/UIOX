#include "TrackInfoWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace UIXPlayer {

TrackInfoWidget::TrackInfoWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void TrackInfoWidget::setupUI()
{
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(8, 4, 8, 4);
    root->setSpacing(12);

    coverLabel_ = new QLabel(this);
    coverLabel_->setFixedSize(56, 56);
    coverLabel_->setScaledContents(true);
    coverLabel_->setStyleSheet(
        "border-radius:6px;"
        "background:#1a1a3e;"
        "border:1px solid #e94560;");
    coverLabel_->setAlignment(Qt::AlignCenter);
    coverLabel_->setText("♫");

    auto* info = new QVBoxLayout;
    info->setSpacing(2);

    titleLabel_ = new QLabel("No Track", this);
    titleLabel_->setObjectName("trackTitle");
    titleLabel_->setWordWrap(false);

    artistLabel_ = new QLabel("–", this);
    artistLabel_->setObjectName("trackArtist");

    albumLabel_ = new QLabel("", this);
    albumLabel_->setObjectName("trackAlbum");

    formatLabel_ = new QLabel("", this);
    formatLabel_->setObjectName("trackAlbum");

    info->addWidget(titleLabel_);
    info->addWidget(artistLabel_);
    info->addWidget(albumLabel_);
    info->addWidget(formatLabel_);

    root->addWidget(coverLabel_);
    root->addLayout(info, 1);
}

void TrackInfoWidget::setTrack(const Track* t)
{
    if (!t) { clear(); return; }

    titleLabel_ ->setText(t->title.isEmpty()  ? "Unknown" : t->title);
    artistLabel_->setText(t->artist.isEmpty() ? "Unknown Artist" : t->artist);
    albumLabel_ ->setText(t->album.isEmpty()  ? "" : t->album);
    formatLabel_->setText(
        QString("[%1  %2kbps  %3kHz]")
            .arg(t->format)
            .arg(t->bit_rate)
            .arg(t->sample_rate / 1000));

    if (!t->cover_art.isNull()) {
        coverLabel_->setPixmap(
            QPixmap::fromImage(t->cover_art).scaled(
                56, 56, Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
    } else {
        coverLabel_->clear();
        coverLabel_->setText("♫");
    }
}

void TrackInfoWidget::clear()
{
    titleLabel_ ->setText("No Track");
    artistLabel_->setText("–");
    albumLabel_ ->setText("");
    formatLabel_->setText("");
    coverLabel_ ->clear();
    coverLabel_ ->setText("♫");
}

} /* namespace UIXPlayer */
