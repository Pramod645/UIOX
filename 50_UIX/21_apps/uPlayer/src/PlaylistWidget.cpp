#include "PlaylistWidget.h"
#include <QListWidgetItem>
#include <QAction>

namespace UIXPlayer {

PlaylistWidget::PlaylistWidget(Playlist& pl, QWidget* parent)
    : QWidget(parent), playlist_(pl)
{
    setupUI();
    connectSignals();
    refresh();
}

void PlaylistWidget::setupUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    /* top bar */
    auto* top = new QHBoxLayout;
    top->setContentsMargins(8, 8, 8, 4);

    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText("Search tracks…");
    searchEdit_->setClearButtonEnabled(true);
    searchEdit_->setStyleSheet(
        "QLineEdit { background:#0f0f23; border:1px solid #333;"
        " border-radius:14px; padding:4px 12px; color:#e0e0e0; }");

    addBtn_ = new QPushButton("+ Add", this);
    addBtn_->setStyleSheet(
        "QPushButton { background:#e94560; border-radius:12px;"
        " padding:4px 14px; font-weight:bold; }"
        "QPushButton:hover { background:#c73652; }");

    menuBtn_ = new QPushButton("⋮", this);
    menuBtn_->setFlat(true);
    menuBtn_->setFixedSize(28, 28);

    top->addWidget(searchEdit_, 1);
    top->addWidget(addBtn_);
    top->addWidget(menuBtn_);
    root->addLayout(top);

    /* playlist view */
    listWidget_ = new QListWidget(this);
    listWidget_->setObjectName("playlistView");
    listWidget_->setContextMenuPolicy(Qt::CustomContextMenu);
    listWidget_->setAttribute(Qt::WA_AcceptTouchEvents);
    listWidget_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    listWidget_->setAlternatingRowColors(false);
    listWidget_->setUniformItemSizes(true);
    root->addWidget(listWidget_, 1);
}

void PlaylistWidget::connectSignals()
{
    connect(listWidget_, &QListWidget::itemDoubleClicked,
            this, &PlaylistWidget::onItemDoubleClicked);
    connect(listWidget_, &QListWidget::customContextMenuRequested,
            this, &PlaylistWidget::showContextMenu);
    connect(searchEdit_, &QLineEdit::textChanged,
            this, &PlaylistWidget::onSearchChanged);
    connect(addBtn_, &QPushButton::clicked,
            this, &PlaylistWidget::addFilesRequested);

    connect(menuBtn_, &QPushButton::clicked, this, [this]{
        QMenu m(this);
        m.addAction("Add Files",   this, &PlaylistWidget::addFilesRequested);
        m.addAction("Add Folder",  this, &PlaylistWidget::addFolderRequested);
        m.addSeparator();
        m.addAction("Save (.m3u)", this, &PlaylistWidget::saveRequested);
        m.addAction("Load (.m3u)", this, &PlaylistWidget::loadRequested);
        m.addSeparator();
        m.addAction("Clear All",   this, &PlaylistWidget::clearRequested);
        m.exec(menuBtn_->mapToGlobal(menuBtn_->rect().bottomLeft()));
    });
}

void PlaylistWidget::populateItem(QListWidgetItem* item,
                                   const Track& t, int idx,
                                   bool isCurrent)
{
    QString text = QString("%1.  %2\n    %3  ·  %4")
        .arg(idx + 1, 2)
        .arg(t.title.isEmpty() ? "(Unknown)" : t.title)
        .arg(t.artist.isEmpty() ? "Unknown Artist" : t.artist)
        .arg(t.durationStr());

    item->setText(text);
    item->setData(Qt::UserRole, idx);

    if (isCurrent) {
        item->setForeground(QColor("#e94560"));
        item->setFont([&]{ auto f = item->font(); f.setBold(true); return f; }());
    }
}

void PlaylistWidget::refresh()
{
    int cur = playlist_.currentIdx();
    listWidget_->clear();
    for (int i = 0; i < playlist_.count(); ++i) {
        const Track* t = playlist_.trackAt(i);
        if (!t) continue;
        auto* item = new QListWidgetItem(listWidget_);
        item->setSizeHint(QSize(0, 54));
        populateItem(item, *t, i, (i == cur));
    }
    highlightCurrent();
}

void PlaylistWidget::highlightCurrent()
{
    int cur = playlist_.currentIdx();
    if (cur >= 0 && cur < listWidget_->count())
        listWidget_->scrollToItem(listWidget_->item(cur));
}

void PlaylistWidget::onItemDoubleClicked(QListWidgetItem* item)
{
    int idx = item->data(Qt::UserRole).toInt();
    emit trackDoubleClicked(idx);
}

void PlaylistWidget::onSearchChanged(const QString& text)
{
    if (text.isEmpty()) {
        for (int i = 0; i < listWidget_->count(); ++i)
            listWidget_->item(i)->setHidden(false);
        return;
    }
    QVector<int> hits = playlist_.search(text);
    for (int i = 0; i < listWidget_->count(); ++i) {
        bool visible = hits.contains(i);
        listWidget_->item(i)->setHidden(!visible);
    }
}

void PlaylistWidget::showContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = listWidget_->itemAt(pos);
    if (!item) return;
    int idx = item->data(Qt::UserRole).toInt();

    QMenu m(this);
    m.addAction("Play",   this, [=]{ emit trackDoubleClicked(idx); });
    m.addSeparator();
    m.addAction("Remove", this, [=]{
        playlist_.removeTrack(idx);
        refresh();
    });
    m.exec(listWidget_->mapToGlobal(pos));
}

} /* namespace UIXPlayer */
