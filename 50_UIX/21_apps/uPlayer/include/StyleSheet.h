#pragma once
#include <QString>

namespace UIXPlayer {

inline QString appStyleSheet()
{
    return R"(
/* ── Global ─────────────────────────────────────────────── */
QWidget {
    background-color: #1a1a2e;
    color: #e0e0e0;
    font-family: "Segoe UI", "SF Pro", "Roboto", sans-serif;
    font-size: 13px;
}

/* ── Main window ─────────────────────────────────────────── */
QMainWindow {
    background-color: #0f0f23;
}

/* ── Menu bar ────────────────────────────────────────────── */
QMenuBar {
    background-color: #16213e;
    color: #e0e0e0;
    padding: 2px;
}
QMenuBar::item:selected {
    background-color: #e94560;
    border-radius: 4px;
}
QMenu {
    background-color: #16213e;
    border: 1px solid #e94560;
    border-radius: 6px;
    padding: 4px;
}
QMenu::item:selected {
    background-color: #e94560;
    border-radius: 4px;
}

/* ── Toolbar ─────────────────────────────────────────────── */
QToolBar {
    background-color: #16213e;
    border: none;
    spacing: 4px;
    padding: 4px;
}
QToolButton {
    background-color: transparent;
    border: none;
    border-radius: 6px;
    padding: 6px;
    color: #e0e0e0;
}
QToolButton:hover {
    background-color: #e94560;
}
QToolButton:pressed {
    background-color: #c73652;
}

/* ── Player controls bar ─────────────────────────────────── */
#controlsBar {
    background-color: #16213e;
    border-top: 1px solid #e94560;
    min-height: 90px;
    padding: 8px;
}

/* ── Control buttons (play/pause/stop/next/prev) ─────────── */
QPushButton#playBtn,
QPushButton#pauseBtn,
QPushButton#stopBtn,
QPushButton#nextBtn,
QPushButton#prevBtn {
    background-color: #16213e;
    border: 2px solid #e94560;
    border-radius: 22px;
    min-width:  44px;
    min-height: 44px;
    max-width:  44px;
    max-height: 44px;
    padding: 0;
}
QPushButton#playBtn  { min-width: 52px; min-height: 52px;
                        max-width: 52px; max-height: 52px;
                        border-radius: 26px; }
QPushButton#playBtn:hover,
QPushButton#pauseBtn:hover,
QPushButton#stopBtn:hover,
QPushButton#nextBtn:hover,
QPushButton#prevBtn:hover {
    background-color: #e94560;
}
QPushButton#playBtn:pressed,
QPushButton#pauseBtn:pressed,
QPushButton#stopBtn:pressed,
QPushButton#nextBtn:pressed,
QPushButton#prevBtn:pressed {
    background-color: #c73652;
}

/* ── Seek slider ─────────────────────────────────────────── */
QSlider#seekSlider::groove:horizontal {
    height: 5px;
    background: #2a2a4a;
    border-radius: 3px;
}
QSlider#seekSlider::sub-page:horizontal {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #e94560, stop:1 #ff6b6b);
    border-radius: 3px;
}
QSlider#seekSlider::handle:horizontal {
    background: #e94560;
    width: 14px;
    height: 14px;
    margin: -5px 0;
    border-radius: 7px;
}
QSlider#seekSlider::handle:horizontal:hover {
    background: #ff6b6b;
    width: 18px;
    height: 18px;
    margin: -7px 0;
    border-radius: 9px;
}

/* ── Volume slider ───────────────────────────────────────── */
QSlider#volumeSlider::groove:horizontal {
    height: 4px;
    background: #2a2a4a;
    border-radius: 2px;
}
QSlider#volumeSlider::sub-page:horizontal {
    background: #e94560;
    border-radius: 2px;
}
QSlider#volumeSlider::handle:horizontal {
    background: #e0e0e0;
    width: 12px;
    height: 12px;
    margin: -4px 0;
    border-radius: 6px;
}

/* ── Playlist widget ─────────────────────────────────────── */
QListWidget#playlistView {
    background-color: #0f0f23;
    border: none;
    outline: none;
}
QListWidget#playlistView::item {
    padding: 10px 14px;
    border-bottom: 1px solid #1a1a3e;
    border-radius: 4px;
}
QListWidget#playlistView::item:selected {
    background-color: #e94560;
    color: #ffffff;
}
QListWidget#playlistView::item:hover:!selected {
    background-color: #222244;
}

/* ── Track info ──────────────────────────────────────────── */
#trackTitle {
    font-size: 16px;
    font-weight: bold;
    color: #ffffff;
}
#trackArtist {
    font-size: 13px;
    color: #e94560;
}
#trackAlbum {
    font-size: 12px;
    color: #888;
}

/* ── Time labels ─────────────────────────────────────────── */
#timeElapsed, #timeDuration {
    font-size: 12px;
    color: #888;
    font-variant-numeric: tabular-nums;
}

/* ── Mode buttons (shuffle/repeat) ──────────────────────── */
QPushButton#shuffleBtn,
QPushButton#repeatBtn {
    background: transparent;
    border: none;
    border-radius: 6px;
    padding: 6px;
    color: #888;
    font-size: 12px;
}
QPushButton#shuffleBtn:checked,
QPushButton#repeatBtn:checked {
    color: #e94560;
    background: #2a1020;
}
QPushButton#shuffleBtn:hover,
QPushButton#repeatBtn:hover {
    background: #222244;
}

/* ── Scroll bars ─────────────────────────────────────────── */
QScrollBar:vertical {
    background: #0f0f23;
    width: 6px;
    border-radius: 3px;
}
QScrollBar::handle:vertical {
    background: #444466;
    border-radius: 3px;
    min-height: 30px;
}
QScrollBar::handle:vertical:hover {
    background: #e94560;
}
QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical { height: 0; }

/* ── Splitter ────────────────────────────────────────────── */
QSplitter::handle {
    background: #e94560;
    width: 1px;
}

/* ── Status bar ──────────────────────────────────────────── */
QStatusBar {
    background: #0f0f23;
    color: #555;
    font-size: 11px;
}

/* ── Tooltip ─────────────────────────────────────────────── */
QToolTip {
    background: #16213e;
    color: #e0e0e0;
    border: 1px solid #e94560;
    border-radius: 4px;
    padding: 4px 8px;
}
)";
}

} /* namespace UIXPlayer */
