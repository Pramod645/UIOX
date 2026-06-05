#pragma once
#include "Playlist.h"
#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>

namespace UIXPlayer {

class PlaylistWidget : public QWidget {
    Q_OBJECT
public:
    explicit PlaylistWidget(Playlist& playlist,
                             QWidget* parent = nullptr);
    void refresh();
    void highlightCurrent();

signals:
    void trackDoubleClicked(int idx);
    void addFilesRequested ();
    void addFolderRequested();
    void clearRequested    ();
    void saveRequested     ();
    void loadRequested     ();

private slots:
    void onItemDoubleClicked(QListWidgetItem* item);
    void onSearchChanged    (const QString& text);
    void showContextMenu    (const QPoint& pos);

private:
    Playlist&    playlist_;
    QListWidget* listWidget_;
    QLineEdit*   searchEdit_;
    QPushButton* addBtn_;
    QPushButton* menuBtn_;

    void setupUI();
    void connectSignals();
    void populateItem(QListWidgetItem* item, const Track& t,
                      int idx, bool isCurrent);
};

} /* namespace UIXPlayer */
