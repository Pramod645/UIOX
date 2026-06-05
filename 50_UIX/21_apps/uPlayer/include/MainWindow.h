#pragma once
#include "Player.h"
#include "PlayerControls.h"
#include "PlaylistWidget.h"
#include "VisualizerWidget.h"
#include <QMainWindow>
#include <QSplitter>
#include <QStackedWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QSettings>

namespace UIXPlayer {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void keyPressEvent  (QKeyEvent*      e) override;
    void dragEnterEvent (QDragEnterEvent* e) override;
    void dropEvent      (QDropEvent*      e) override;
    void closeEvent     (QCloseEvent*     e) override;
    void resizeEvent    (QResizeEvent*    e) override;
    bool eventFilter    (QObject* obj, QEvent* e) override;

private slots:
    /* player signals */
    void onStateChanged      (QMediaPlayer::PlaybackState s);
    void onPositionChanged   (qint64 ms);
    void onDurationChanged   (qint64 ms);
    void onTrackChanged      (const Track* t);
    void onVolumeChanged     (float v);
    void onShuffleModeChanged(ShuffleMode m);
    void onRepeatModeChanged (RepeatMode  m);
    void onError             (const QString& msg);

    /* controls signals */
    void onPlayClicked  ();
    void onPauseClicked ();
    void onStopClicked  ();
    void onNextClicked  ();
    void onPrevClicked  ();
    void onSeekRequested(qint64 ms);

    /* playlist signals */
    void onAddFiles   ();
    void onAddFolder  ();
    void onClear      ();
    void onSavePlaylist();
    void onLoadPlaylist();
    void onTrackDoubleClicked(int idx);

    /* menu actions */
    void openFiles       ();
    void openFolder      ();
    void openURL         ();
    void quit            ();
    void showAbout       ();
    void toggleFullscreen();
    void togglePlaylist  ();

private:
    Player*           player_;
    PlayerControls*   controls_;
    PlaylistWidget*   playlistWidget_;
    VisualizerWidget* visualizer_;
    QSplitter*        splitter_;
    QWidget*          centerWidget_;
    QSettings         settings_;

    void setupUI           ();
    void setupMenuBar      ();
    void setupToolBar      ();
    void setupStatusBar    ();
    void connectSignals    ();
    void addFilesToPlaylist(const QList<QUrl>& urls);
    void saveSettings      ();
    void restoreSettings   ();

    /* touch gesture support */
    void setupGestures();
};

} /* namespace UIXPlayer */
