#pragma once
#include <QWidget>
#include <QSlider>
#include <QPushButton>
#include <QHBoxLayout>

namespace UIXPlayer {

class VolumeSlider : public QWidget {
    Q_OBJECT
public:
    explicit VolumeSlider(QWidget* parent = nullptr);

    float volume() const;
    void  setVolume(float v);

signals:
    void volumeChanged(float v);
    void muteToggled  (bool muted);

private:
    QPushButton* muteBtn_;
    QSlider*     slider_;
    bool         muted_ = false;
    float        lastVol_ = 0.8f;

    void setupUI();
    void updateMuteIcon();
};

} /* namespace UIXPlayer */
