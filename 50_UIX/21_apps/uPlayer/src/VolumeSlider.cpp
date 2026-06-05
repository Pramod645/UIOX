#include "VolumeSlider.h"

namespace UIXPlayer {

VolumeSlider::VolumeSlider(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void VolumeSlider::setupUI()
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    muteBtn_ = new QPushButton(this);
    muteBtn_->setFixedSize(28, 28);
    muteBtn_->setObjectName("muteBtn");
    muteBtn_->setToolTip("Mute");
    muteBtn_->setText("🔊");
    muteBtn_->setFlat(true);

    slider_ = new QSlider(Qt::Horizontal, this);
    slider_->setObjectName("volumeSlider");
    slider_->setRange(0, 100);
    slider_->setValue(80);
    slider_->setFixedWidth(100);
    slider_->setToolTip("Volume");
    setAttribute(Qt::WA_AcceptTouchEvents);

    layout->addWidget(muteBtn_);
    layout->addWidget(slider_);

    connect(muteBtn_, &QPushButton::clicked, this, [this]{
        muted_ = !muted_;
        if (muted_) {
            lastVol_ = volume();
            slider_->setValue(0);
            muteBtn_->setText("🔇");
        } else {
            slider_->setValue((int)(lastVol_ * 100));
            muteBtn_->setText("🔊");
        }
        emit muteToggled(muted_);
    });

    connect(slider_, &QSlider::valueChanged, this, [this](int v){
        float vol = v / 100.0f;
        if (!muted_) lastVol_ = vol;
        emit volumeChanged(vol);
    });
}

float VolumeSlider::volume() const
{
    return slider_->value() / 100.0f;
}

void VolumeSlider::setVolume(float v)
{
    slider_->setValue((int)(v * 100));
}

void VolumeSlider::updateMuteIcon()
{
    muteBtn_->setText(muted_ ? "🔇" : "🔊");
}

} /* namespace UIXPlayer */
