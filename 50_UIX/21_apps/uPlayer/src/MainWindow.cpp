#include "MainWindow.h"
#include "StyleSheet.h"
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QKeyEvent>
#include <QGestureEvent>
#include <QSwipeGesture>
#include <QTapGesture>
#include <QMenuBar>
#include <QStatusBar>
#include <QApplication>
#include <QScreen>
#include <QTimer>

namespace UIXPlayer {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , settings_("UIXPlayer", "UIXPlayer")
{
    setWindowTitle("UIX Player");
    setMinimumSize(700, 500);
    setAcceptDrops(true);

    player_ = new Player(this);
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    connectSignals();
    setupGestures();
    restoreSettings();

    /* demo: add some tracks */
    auto& pl = player_->playlist();
    auto addDemo = [&](const QString& title, const QString& artist,
                        const QString& album, const QString& file) {
        Track t;
        t.title  = title;
        t.artist = artist;
        t.album  = album;
        t.url    = QUrl::fromLocalFile(file);
        t.format = "MP3";
        t.duration_ms = 240000;
        pl.addTrack(t);
    };
    addDemo("Bohemian Rhapsody",    "Queen",
            "A Night at the Opera", "/music/bohemian.mp3");
    addDemo("Hotel California",     "Eagles",
            "Hotel California",     "/music/hotel.mp3");
    addDemo("Stairway to Heaven",   "Led Zeppelin",
            "Led Zeppelin IV",      "/music/stairway.mp3");
    addDemo("Smells Like Teen Spirit","Nirvana",
            "Nevermind",            "/music/nirvana.mp3");
    addDemo("Billie Jean",          "Michael Jackson",
            "Thriller",             "/music/billie.mp3");
    addDemo("Purple Rain",          "Prince",
            "Purple Rain",          "/music/purple.mp3");

    playlistWidget_->refresh();
}

MainWindow::~MainWindow()
{
    saveSettings();
}

void MainWindow::setupUI()
{
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    /* splitter: visualizer + center | playlist */
    splitter_ = new QSplitter(Qt::Horizontal, central);

    /* left: visualizer + center */
    centerWidget_ = new QWidget(splitter_);
    auto* centerLayout = new QVBoxLayout(centerWidget_);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(0);

    visualizer_ = new VisualizerWidget(centerWidget_);
    centerLayout->addWidget(visualizer_);

    /* album art placeholder */
    auto* artLabel = new QLabel(centerWidget_);
    artLabel->setAlignment(Qt::AlignCenter);
    artLabel->setText("🎵");
    artLabel->setStyleSheet(
        "font-size:80px; color:#2a2a5a;"
        "background:#0a0a1e;");
    artLabel->setSizePolicy(QSizePolicy::Expanding,
                             QSizePolicy::Expanding);
    centerLayout->addWidget(artLabel, 1);

    splitter_->addWidget(centerWidget_);

    /* right: playlist */
    playlistWidget_ = new PlaylistWidget(player_->playlist(),
                                          splitter_);
    splitter_->addWidget(playlistWidget_);
    splitter_->setStretchFactor(0, 2);
    splitter_->setStretchFactor(1, 1);
    splitter_->setSizes({500, 280});

    root->addWidget(splitter_, 1);

    /* controls at bottom */
    controls_ = new PlayerControls(*player_, central);
    root->addWidget(controls_);
}

void MainWindow::setupMenuBar()
{
    auto* mb = menuBar();

    /* Media menu */
    auto* media = mb->addMenu("&Media");
    media->addAction("Open File(s)…", this,
                      &MainWindow::openFiles,
                      QKeySequence("Ctrl+O"));
    media->addAction("Open Folder…", this,
                      &MainWindow::openFolder,
                      QKeySequence("Ctrl+F"));
    media->addAction("Open URL…",    this,
                      &MainWindow::openURL,
                      QKeySequence("Ctrl+U"));
    media->addSeparator();
    media->addAction("Save Playlist…", this,
                      &MainWindow::onSavePlaylist);
    media->addAction("Load Playlist…", this,
                      &MainWindow::onLoadPlaylist);
    media->addSeparator();
    media->addAction("&Quit", this, &MainWindow::quit,
                      QKeySequence("Ctrl+Q"));

    /* Playback menu */
    auto* pb = mb->addMenu("&Playback");
    pb->addAction("Play/Pause",  this, &MainWindow::onPlayClicked,
                   QKeySequence(Qt::Key_Space));
    pb->addAction("Stop",        this, &MainWindow::onStopClicked,
                   QKeySequence(Qt::Key_S));
    pb->addAction("Next",        this, &MainWindow::onNextClicked,
                   QKeySequence(Qt::Key_Right));
    pb->addAction("Previous",    this, &MainWindow::onPrevClicked,
                   QKeySequence(Qt::Key_Left));
    pb->addSeparator();
    pb->addAction("Toggle Shuffle", this, [this]{
        player_->toggleShuffle(); });
    pb->addAction("Cycle Repeat",   this, [this]{
        player_->cycleRepeat(); });

    /* View menu */
    auto* view = mb->addMenu("&View");
    view->addAction("Toggle Playlist", this,
                     &MainWindow::togglePlaylist,
                     QKeySequence("Ctrl+L"));
    view->addAction("Toggle Fullscreen", this,
                     &MainWindow::toggleFullscreen,
                     QKeySequence("Ctrl+Shift+F"));

    /* Help menu */
    auto* help = mb->addMenu("&Help");
    help->addAction("About UIX Player", this, &MainWindow::showAbout);
}

void MainWindow::setupToolBar()
{
    auto* tb = addToolBar("Quick");
    tb->setMovable(false);
    tb->setIconSize(QSize(20, 20));

    tb->addAction("📂 Open", this, &MainWindow::openFiles);
    tb->addAction("📋 Load Playlist", this, &MainWindow::onLoadPlaylist);
    tb->addSeparator();
    tb->addAction("⏮", this, &MainWindow::onPrevClicked);
    tb->addAction("▶", this, &MainWindow::onPlayClicked);
    tb->addAction("⏹", this, &MainWindow::onStopClicked);
    tb->addAction("⏭", this, &MainWindow::onNextClicked);
}

void MainWindow::setupStatusBar()
{
    statusBar()->showMessage("UIX Player ready  —  Drop files to play");
}

void MainWindow::setupGestures()
{
    grabGesture(Qt::SwipeGesture);
    grabGesture(Qt::TapGesture);
}

void MainWindow::connectSignals()
{
    /* player → window */
    connect(player_, &Player::stateChanged,
            this, &MainWindow::onStateChanged);
    connect(player_, &Player::positionChanged,
            this, &MainWindow::onPositionChanged);
    connect(player_, &Player::durationChanged,
            this, &MainWindow::onDurationChanged);
    connect(player_, &Player::trackChanged,
            this, &MainWindow::onTrackChanged);
    connect(player_, &Player::volumeChanged,
            this, &MainWindow::onVolumeChanged);
    connect(player_, &Player::shuffleModeChanged,
            this, &MainWindow::onShuffleModeChanged);
    connect(player_, &Player::repeatModeChanged,
            this, &MainWindow::onRepeatModeChanged);
    connect(player_, &Player::errorOccurred,
            this, &MainWindow::onError);

    /* controls → player */
    connect(controls_, &PlayerControls::playClicked,
            this, &MainWindow::onPlayClicked);
    connect(controls_, &PlayerControls::pauseClicked,
            this, &MainWindow::onPauseClicked);
    connect(controls_, &PlayerControls::stopClicked,
            this, &MainWindow::onStopClicked);
    connect(controls_, &PlayerControls::nextClicked,
            this, &MainWindow::onNextClicked);
    connect(controls_, &PlayerControls::prevClicked,
            this, &MainWindow::onPrevClicked);
    connect(controls_, &PlayerControls::seekRequested,
            this, &MainWindow::onSeekRequested);
    connect(controls_, &PlayerControls::volumeChanged,
            player_, &Player::setVolume);
    connect(controls_, &PlayerControls::muteToggled,
            player_, &Player::setMuted);
    connect(controls_, &PlayerControls::shuffleClicked,
            player_, &Player::toggleShuffle);
    connect(controls_, &PlayerControls::repeatClicked,
            player_, &Player::cycleRepeat);

    /* playlist widget */
    connect(playlistWidget_, &PlaylistWidget::trackDoubleClicked,
            this, &MainWindow::onTrackDoubleClicked);
    connect(playlistWidget_, &PlaylistWidget::addFilesRequested,
            this, &MainWindow::onAddFiles);
    connect(playlistWidget_, &PlaylistWidget::addFolderRequested,
            this, &MainWindow::onAddFolder);
    connect(playlistWidget_, &PlaylistWidget::clearRequested,
            this, &MainWindow::onClear);
    connect(playlistWidget_, &PlaylistWidget::saveRequested,
            this, &MainWindow::onSavePlaylist);
    connect(playlistWidget_, &PlaylistWidget::loadRequested,
            this, &MainWindow::onLoadPlaylist);
}

/* ── Player signal handlers ──────────────────────────────── */
void MainWindow::onStateChanged(QMediaPlayer::PlaybackState s)
{
    controls_->updateState(s);
    if (s == QMediaPlayer::PlayingState) {
        visualizer_->start();
        statusBar()->showMessage("Playing…");
    } else if (s == QMediaPlayer::PausedState) {
        visualizer_->pause();
        statusBar()->showMessage("Paused");
    } else {
        visualizer_->stop();
        statusBar()->showMessage("Stopped");
    }
}

void MainWindow::onPositionChanged(qint64 ms)
{
    controls_->updatePosition(ms);
}

void MainWindow::onDurationChanged(qint64 ms)
{
    controls_->updateDuration(ms);
}

void MainWindow::onTrackChanged(const Track* t)
{
    controls_->updateTrack(t);
    if (t) {
        setWindowTitle(
            QString("UIX Player  —  %1  ·  %2")
                .arg(t->title, t->artist));
        statusBar()->showMessage(
            QString("Now playing: %1").arg(t->title));
    }
    playlistWidget_->highlightCurrent();
}

void MainWindow::onVolumeChanged(float v)
{
    controls_->updateVolume(v);
}

void MainWindow::onShuffleModeChanged(ShuffleMode m)
{
    controls_->updateShuffle(m);
}

void MainWindow::onRepeatModeChanged(RepeatMode m)
{
    controls_->updateRepeat(m);
}

void MainWindow::onError(const QString& msg)
{
    statusBar()->showMessage("Error: " + msg, 5000);
}

/* ── Controls signal handlers ────────────────────────────── */
void MainWindow::onPlayClicked()
{
    auto s = player_->state();
    if (s == QMediaPlayer::PlayingState) player_->pause();
    else                                  player_->play();
}

void MainWindow::onPauseClicked()  { player_->pause(); }
void MainWindow::onStopClicked()   { player_->stop();  }
void MainWindow::onNextClicked()   { player_->nextTrack(); }
void MainWindow::onPrevClicked()   { player_->prevTrack(); }

void MainWindow::onSeekRequested(qint64 ms)
{
    player_->seek(ms);
}

/* ── Playlist handlers ───────────────────────────────────── */
void MainWindow::onAddFiles()   { openFiles();  }
void MainWindow::onAddFolder()  { openFolder(); }

void MainWindow::onClear()
{
    player_->stop();
    player_->playlist().clear();
    playlistWidget_->refresh();
}

void MainWindow::onSavePlaylist()
{
    QString path = QFileDialog::getSaveFileName(
        this, "Save Playlist", QDir::homePath(),
        "M3U Playlist (*.m3u)");
    if (!path.isEmpty()) {
        player_->playlist().saveM3U(path);
        statusBar()->showMessage("Playlist saved: " + path, 3000);
    }
}

void MainWindow::onLoadPlaylist()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Load Playlist", QDir::homePath(),
        "M3U Playlist (*.m3u)");
    if (!path.isEmpty()) {
        player_->playlist().loadM3U(path);
        playlistWidget_->refresh();
        statusBar()->showMessage("Playlist loaded: " + path, 3000);
    }
}

void MainWindow::onTrackDoubleClicked(int idx)
{
    player_->playlist().seekTo(idx);
    player_->play();
    playlistWidget_->highlightCurrent();
}

/* ── Menu actions ────────────────────────────────────────── */
void MainWindow::openFiles()
{
    QList<QUrl> urls = QFileDialog::getOpenFileUrls(
        this, "Open Audio Files", QUrl::fromLocalFile(QDir::homePath()),
        "Audio Files (*.mp3 *.flac *.wav *.ogg *.aac *.m4a *.opus);;"
        "All Files (*)");
    if (!urls.isEmpty())
        addFilesToPlaylist(urls);
}

void MainWindow::openFolder()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, "Open Folder", QDir::homePath());
    if (dir.isEmpty()) return;

    QStringList filters = {"*.mp3","*.flac","*.wav",
                            "*.ogg","*.aac","*.m4a","*.opus"};
    QDirIterator it(dir, filters,
                    QDir::Files, QDirIterator::Subdirectories);
    QList<QUrl> urls;
    while (it.hasNext())
        urls.append(QUrl::fromLocalFile(it.next()));
    if (!urls.isEmpty())
        addFilesToPlaylist(urls);
}

void MainWindow::openURL()
{
    QString url = QInputDialog::getText(
        this, "Open URL", "Enter stream URL:",
        QLineEdit::Normal, "http://");
    if (url.isEmpty()) return;
    Track t;
    t.title  = url;
    t.url    = QUrl(url);
    t.format = "Stream";
    player_->playlist().addTrack(t);
    playlistWidget_->refresh();
}

void MainWindow::addFilesToPlaylist(const QList<QUrl>& urls)
{
    for (const QUrl& u : urls) {
        QFileInfo fi(u.toLocalFile());
        Track t;
        t.title   = fi.baseName();
        t.url     = u;
        t.format  = fi.suffix().toUpper();
        player_->playlist().addTrack(t);
    }
    playlistWidget_->refresh();
    if (player_->state() == QMediaPlayer::StoppedState)
        player_->play();
    statusBar()->showMessage(
        QString("Added %1 file(s)").arg(urls.size()), 3000);
}

void MainWindow::quit()
{
    saveSettings();
    qApp->quit();
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "About UIX Player",
        "<h2>UIX Player 1.0</h2>"
        "<p>A cross-platform music player built with C++ and Qt6.</p>"
        "<p>Platforms: macOS · Windows · Android</p>"
        "<p>Controls: Touch · Mouse · Keyboard</p>");
}

void MainWindow::toggleFullscreen()
{
    if (isFullScreen()) showNormal();
    else                showFullScreen();
}

void MainWindow::togglePlaylist()
{
    playlistWidget_->setVisible(!playlistWidget_->isVisible());
}

/* ── Event overrides ─────────────────────────────────────── */
void MainWindow::keyPressEvent(QKeyEvent* e)
{
    switch (e->key()) {
        case Qt::Key_Space:
            onPlayClicked();
            break;
        case Qt::Key_Right:
            if (e->modifiers() & Qt::ShiftModifier)
                player_->seek(player_->position() + 10000);
            else
                player_->nextTrack();
            break;
        case Qt::Key_Left:
            if (e->modifiers() & Qt::ShiftModifier)
                player_->seek(
                    qMax(0LL, player_->position() - 10000));
            else
                player_->prevTrack();
            break;
        case Qt::Key_Up:
            player_->setVolume(
                qMin(1.0f, player_->volume() + 0.05f));
            break;
        case Qt::Key_Down:
            player_->setVolume(
                qMax(0.0f, player_->volume() - 0.05f));
            break;
        case Qt::Key_S:
            player_->stop();
            break;
        case Qt::Key_R:
            player_->cycleRepeat();
            break;
        case Qt::Key_X:
            player_->toggleShuffle();
            break;
        case Qt::Key_M:
            player_->setMuted(!player_->isMuted());
            break;
        case Qt::Key_F:
        case Qt::Key_F11:
            toggleFullscreen();
            break;
        case Qt::Key_Escape:
            if (isFullScreen()) showNormal();
            break;
        default:
            QMainWindow::keyPressEvent(e);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent* e)
{
    if (e->mimeData()->hasUrls())
        e->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* e)
{
    addFilesToPlaylist(e->mimeData()->urls());
}

void MainWindow::closeEvent(QCloseEvent* e)
{
    saveSettings();
    e->accept();
}

void MainWindow::resizeEvent(QResizeEvent* e)
{
    QMainWindow::resizeEvent(e);
}

bool MainWindow::eventFilter(QObject* obj, QEvent* e)
{
    return QMainWindow::eventFilter(obj, e);
}

void MainWindow::saveSettings()
{
    settings_.setValue("geometry",       saveGeometry());
    settings_.setValue("windowState",    saveState());
    settings_.setValue("splitterState",  splitter_->saveState());
    settings_.setValue("volume",         player_->volume());
}

void MainWindow::restoreSettings()
{
    restoreGeometry(settings_.value("geometry").toByteArray());
    restoreState   (settings_.value("windowState").toByteArray());
    splitter_->restoreState(
        settings_.value("splitterState").toByteArray());
    float vol = settings_.value("volume", 0.8f).toFloat();
    player_->setVolume(vol);
    controls_->updateVolume(vol);
}

} /* namespace UIXPlayer */
