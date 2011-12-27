/***********************************************************************************
* Mini Player: Advanced media player for Plasma.
* Copyright (C) 2008 - 2011 Michal Dutkiewicz aka Emdek <emdeck@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*
***********************************************************************************/

#include "PlaylistManager.h"
#include "PlaylistModel.h"
#include "MetaDataManager.h"
#include "Player.h"

#include <QtCore/QFile>
#include <QtGui/QKeyEvent>
#include <QtGui/QClipboard>
#include <QtGui/QHeaderView>
#include <QtGui/QContextMenuEvent>

#include <KMenu>
#include <KMessageBox>
#include <KFileDialog>
#include <KInputDialog>

#include <Plasma/Theme>

namespace MiniPlayer
{

PlaylistManager::PlaylistManager(Player *parent) : QObject(parent),
    m_player(parent),
    m_dialog(NULL),
    m_currentPlaylist(0),
    m_editorActive(false)
{
}

void PlaylistManager::addTracks(const KUrl::List &tracks, int index, bool play)
{
    m_playlists[visiblePlaylist()]->addTracks(tracks, index, play);
}

void PlaylistManager::trackPressed()
{
    if (!m_playlistUi.playlistView->selectionModel())
    {
        return;
    }

    QModelIndexList selectedIndexes = m_playlistUi.playlistView->selectionModel()->selectedIndexes();
    bool hasTracks = m_playlists[visiblePlaylist()]->trackCount();

    m_playlistUi.addButton->setEnabled(!m_playlists[visiblePlaylist()]->isReadOnly());
    m_playlistUi.removeButton->setEnabled(!selectedIndexes.isEmpty());
    m_playlistUi.editButton->setEnabled(!selectedIndexes.isEmpty() && !m_playlists[visiblePlaylist()]->isReadOnly());
    m_playlistUi.moveUpButton->setEnabled((m_playlists[visiblePlaylist()]->trackCount() > 1) && !selectedIndexes.isEmpty() && selectedIndexes.first().row() != 0);
    m_playlistUi.moveDownButton->setEnabled((m_playlists[visiblePlaylist()]->trackCount() > 1) && !selectedIndexes.isEmpty() && selectedIndexes.last().row() != (m_playlists[visiblePlaylist()]->trackCount() - 1));
    m_playlistUi.clearButton->setEnabled(hasTracks && !m_playlists[visiblePlaylist()]->isReadOnly());
    m_playlistUi.playbackModeButton->setEnabled(hasTracks);
    m_playlistUi.exportButton->setEnabled(hasTracks && !m_playlists[visiblePlaylist()]->isReadOnly());
    m_playlistUi.shuffleButton->setEnabled((m_playlists[visiblePlaylist()]->trackCount() > 1) && !m_playlists[visiblePlaylist()]->isReadOnly());
    m_playlistUi.playlistViewFilter->setEnabled(hasTracks);

    m_editorActive = false;
}

void PlaylistManager::trackChanged()
{
    m_editorActive = false;
}

void PlaylistManager::moveUpTrack()
{
    KUrl url(m_playlistUi.playlistView->currentIndex().data(Qt::ToolTipRole).toString());
    int row = m_playlistUi.playlistView->currentIndex().row();

    m_playlists[visiblePlaylist()]->removeTrack(row);
    m_playlists[visiblePlaylist()]->addTrack((row - 1), url);

    m_playlistUi.playlistView->setCurrentIndex(m_playlists[visiblePlaylist()]->index((row - 1), 0));

    trackPressed();
}

void PlaylistManager::moveDownTrack()
{
    KUrl url(m_playlistUi.playlistView->currentIndex().data(Qt::ToolTipRole).toString());
    int row = m_playlistUi.playlistView->currentIndex().row();

    m_playlists[visiblePlaylist()]->removeTrack(row);
    m_playlists[visiblePlaylist()]->addTrack((row + 1), url);

    m_playlistUi.playlistView->setCurrentIndex(m_playlists[visiblePlaylist()]->index((row + 1), 0));

    trackPressed();
}

void PlaylistManager::playTrack(QModelIndex index)
{
    if (!index.isValid())
    {
        index = m_playlistUi.playlistView->currentIndex();
    }

    if (visiblePlaylist() != m_currentPlaylist)
    {
        setCurrentPlaylist(visiblePlaylist());
    }

    m_player->play(index.row());
}

void PlaylistManager::editTrackTitle()
{
    m_editorActive = true;

    m_playlistUi.playlistView->edit(m_playlistUi.playlistView->currentIndex());
}

void PlaylistManager::copyTrackUrl()
{
    QApplication::clipboard()->setText(m_playlistUi.playlistView->currentIndex().data(Qt::ToolTipRole).toString());
}

void PlaylistManager::removeTrack()
{
    int row = m_playlistUi.playlistView->currentIndex().row();

    m_playlists[visiblePlaylist()]->removeTrack(row);

    m_playlistUi.playlistView->setCurrentIndex(m_playlists[visiblePlaylist()]->index(row, 0));
}

void PlaylistManager::movePlaylist(int from, int to)
{
///FIXME validate
    m_playlists.swap(from, to);

    emit configNeedsSaving();
}

void PlaylistManager::renamePlaylist(int position)
{
    QString oldTitle = KGlobal::locale()->removeAcceleratorMarker(m_playlistUi.tabBar->tabText(position));
    QString newTitle = KInputDialog::getText(i18n("Rename Playlist"), i18n("Enter name:"), oldTitle);

///FIXME add plaistnames method?
    if (newTitle.isEmpty())// || m_playlists.contains(newTitle))
    {
        return;
    }

    m_playlistUi.tabBar->setTabText(position, newTitle);
    m_playlists[position]->setTitle(newTitle);

    emit configNeedsSaving();
}

void PlaylistManager::removePlaylist(int position)
{
    if (m_playlistUi.tabBar->count() == 1)
    {
        clearPlaylist();

        return;
    }

    QString oldTitle = KGlobal::locale()->removeAcceleratorMarker(m_playlistUi.tabBar->tabText(position));

    m_playlistUi.tabBar->removeTab(position);

    m_playlists[position]->deleteLater();
    m_playlists.removeAt(position);

    if (m_currentPlaylist == position)
    {
        setCurrentPlaylist(qMax(0, (position - 1)));
    }

    if (m_playlistUi.tabBar->count() == 1)
    {
        m_playlistUi.tabBar->hide();
    }

    emit configNeedsSaving();
}

void PlaylistManager::visiblePlaylistChanged(int position)
{
    if (position > m_playlists.count())
    {
        return;
    }

    if (!m_playlistUi.playlistViewFilter->text().isEmpty())
    {
        filterPlaylist(m_playlistUi.playlistViewFilter->text());
    }

    if (m_player->state() != PlayingState && m_player->state() != PausedState)
    {
        setCurrentPlaylist(position);
    }

    m_playlistUi.playlistView->setModel(m_playlists[visiblePlaylist()]);
    m_playlistUi.playlistView->horizontalHeader()->setResizeMode(0, QHeaderView::Fixed);
    m_playlistUi.playlistView->horizontalHeader()->resizeSection(0, 22);
    m_playlistUi.playlistView->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);

    emit configNeedsSaving();
}

void PlaylistManager::showDialog(const QPoint &position)
{
    if (!m_dialog)
    {
        m_dialog = new Plasma::Dialog(NULL, Qt::Tool);
        m_dialog->setFocusPolicy(Qt::NoFocus);
        m_dialog->setWindowTitle(i18n("Playlist"));
        m_dialog->setWindowIcon(KIcon("applications-multimedia"));
        m_dialog->setResizeHandleCorners(Plasma::Dialog::All);

        m_playlistUi.setupUi(m_dialog);

        m_dialog->setContentsMargins(0, 0, 0, 0);
        m_dialog->adjustSize();

        m_playlistUi.videoOutputWidget->installEventFilter(this);
        m_playlistUi.playlistView->installEventFilter(this);
        m_playlistUi.blankLabel->setPixmap(KIcon("applications-multimedia").pixmap(KIconLoader::SizeEnormous));
        m_playlistUi.closeButton->setIcon(KIcon("window-close"));
        m_playlistUi.addButton->setIcon(KIcon("list-add"));
        m_playlistUi.removeButton->setIcon(KIcon("list-remove"));
        m_playlistUi.editButton->setIcon(KIcon("document-edit"));
        m_playlistUi.moveUpButton->setIcon(KIcon("arrow-up"));
        m_playlistUi.moveDownButton->setIcon(KIcon("arrow-down"));
        m_playlistUi.newButton->setIcon(KIcon("document-new"));
        m_playlistUi.exportButton->setIcon(KIcon("document-export"));
        m_playlistUi.clearButton->setIcon(KIcon("edit-clear"));
        m_playlistUi.shuffleButton->setIcon(KIcon("roll"));
        m_playlistUi.playbackModeButton->setDefaultAction(m_player->action(PlaybackModeMenuAction));
        m_playlistUi.playPauseButton->setDefaultAction(m_player->action(PlayPauseAction));
        m_playlistUi.stopButton->setDefaultAction(m_player->action(StopAction));
        m_playlistUi.fullScreenButton->setDefaultAction(m_player->action(FullScreenAction));
        m_playlistUi.seekSlider->setPlayer(m_player);
        m_playlistUi.muteButton->setDefaultAction(m_player->action(MuteAction));
        m_playlistUi.volumeSlider->setPlayer(m_player);
        m_playlistUi.splitter->setStretchFactor(0, 1);
        m_playlistUi.splitter->setStretchFactor(1, 3);
        m_playlistUi.splitter->setStretchFactor(2, 1);
        m_playlistUi.splitter->setStretchFactor(3, 3);
        m_playlistUi.splitter->setStretchFactor(4, 1);
        m_playlistUi.splitter->setStretchFactor(5, 1);

        for (int i = 0; i < m_playlists.count(); ++i)
        {
            m_playlistUi.tabBar->addTab(KIcon((m_currentPlaylist == i)?"media-playback-start":"view-media-playlist"), m_playlists.at(i)->title());
        }

        m_playlistUi.tabBar->setVisible(m_playlists.count() > 1);

        updateTheme();
        trackPressed();
        visiblePlaylistChanged(m_currentPlaylist);

        connect(m_dialog, SIGNAL(dialogResized()), this, SIGNAL(configNeedsSaving()));
        connect(m_playlistUi.splitter, SIGNAL(splitterMoved(int,int)), this, SIGNAL(configNeedsSaving()));
        connect(m_playlistUi.tabBar, SIGNAL(newTabRequest()), this, SLOT(newPlaylist()));
        connect(m_playlistUi.tabBar, SIGNAL(tabDoubleClicked(int)), this, SLOT(renamePlaylist(int)));
        connect(m_playlistUi.tabBar, SIGNAL(closeRequest(int)), this, SLOT(removePlaylist(int)));
        connect(m_playlistUi.tabBar, SIGNAL(currentChanged(int)), this, SLOT(visiblePlaylistChanged(int)));
        connect(m_playlistUi.tabBar, SIGNAL(tabMoved(int,int)), this, SLOT(movePlaylist(int,int)));
        connect(m_playlistUi.closeButton, SIGNAL(clicked()), m_dialog, SLOT(close()));
        connect(m_playlistUi.addButton, SIGNAL(clicked()), m_player->action(OpenFileAction), SLOT(trigger()));
        connect(m_playlistUi.removeButton, SIGNAL(clicked()), this, SLOT(removeTrack()));
        connect(m_playlistUi.editButton, SIGNAL(clicked()), this, SLOT(editTrackTitle()));
        connect(m_playlistUi.moveUpButton, SIGNAL(clicked()), this, SLOT(moveUpTrack()));
        connect(m_playlistUi.moveDownButton, SIGNAL(clicked()), this, SLOT(moveDownTrack()));
        connect(m_playlistUi.newButton, SIGNAL(clicked()), this, SLOT(newPlaylist()));
        connect(m_playlistUi.exportButton, SIGNAL(clicked()), this, SLOT(exportPlaylist()));
        connect(m_playlistUi.clearButton, SIGNAL(clicked()), this, SLOT(clearPlaylist()));
        connect(m_playlistUi.shuffleButton, SIGNAL(clicked()), this, SLOT(shufflePlaylist()));
        connect(m_playlistUi.playlistView, SIGNAL(pressed(QModelIndex)), this, SLOT(trackPressed()));
        connect(m_playlistUi.playlistView, SIGNAL(activated(QModelIndex)), this, SLOT(playTrack(QModelIndex)));
        connect(m_playlistUi.playlistViewFilter, SIGNAL(textChanged(QString)), this, SLOT(filterPlaylist(QString)));
        connect(m_player->parent(), SIGNAL(titleChanged(QString)), m_playlistUi.titleLabel, SLOT(setText(QString)));
        connect(this, SIGNAL(destroyed()), m_dialog, SLOT(deleteLater()));
        connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(updateTheme()));
    }

    m_dialog->move(position);
    m_dialog->show();

    trackPressed();
}

void PlaylistManager::closeDialog()
{
    if (m_dialog)
    {
        m_dialog->close();
    }
}

void PlaylistManager::filterPlaylist(const QString &text)
{
    for (int i = 0; i < m_playlists[visiblePlaylist()]->trackCount(); ++i)
    {
        m_playlistUi.playlistView->setRowHidden(i, (!text.isEmpty() && !m_playlists[visiblePlaylist()]->index(i, 1).data(Qt::DisplayRole).toString().contains(text, Qt::CaseInsensitive)));
    }
}

void PlaylistManager::createPlaylist(const QString &title, const KUrl::List &tracks)
{
///FIXME
    if (title.isEmpty())// || m_playlists.contains(title))
    {
        return;
    }

    PlaylistModel *playlist = new PlaylistModel(m_player, title);
    int position = (visiblePlaylist() + 1);

    m_playlists.insert(position, playlist);

    if (!tracks.isEmpty())
    {
        m_playlists[position]->addTracks(tracks);

        m_player->metaDataManager()->addTracks(tracks);
    }

    if (m_dialog)
    {
        m_playlistUi.tabBar->show();
        m_playlistUi.tabBar->insertTab((position + 1), KIcon("view-media-playlist"), title);
        m_playlistUi.tabBar->setCurrentIndex(position + 1);
    }

    emit configNeedsSaving();

    connect(playlist, SIGNAL(needsSaving()), this, SIGNAL(configNeedsSaving()));
    connect(playlist, SIGNAL(itemChanged(QModelIndex)), this, SLOT(trackChanged()));
}

void PlaylistManager::exportPlaylist()
{
    KFileDialog dialog(KUrl("~"), QString(), NULL);
    dialog.setMimeFilter(QStringList() << "audio/x-scpls" << "audio/x-mpegurl");
    dialog.setWindowModality(Qt::NonModal);
    dialog.setMode(KFile::File);
    dialog.setOperationMode(KFileDialog::Saving);
    dialog.setConfirmOverwrite(true);
    dialog.setSelection(m_playlists[visiblePlaylist()]->title() + ".pls");
    dialog.exec();

    if (dialog.selectedUrl().isEmpty())
    {
        return;
    }

    QFile data(dialog.selectedUrl().toLocalFile());

    if (data.open(QFile::WriteOnly))
    {
        QTextStream out(&data);
        QString title;
        QString duration;
        KUrl url;
        PlaylistType type = (dialog.selectedUrl().toLocalFile().endsWith(".pls")?PLS:M3U);
        bool available;

        if (type == PLS)
        {
            out << "[playlist]\n";
            out << "NumberOfEntries=" << m_playlists[visiblePlaylist()]->trackCount() << "\n\n";
        }
        else
        {
            out << "#EXTM3U\n\n";
        }

        for (int i = 0; i < m_playlists[visiblePlaylist()]->trackCount(); ++i)
        {
            available = m_player->metaDataManager()->available(url);
            url = m_playlists[visiblePlaylist()]->track(i);
            title = (available?m_player->metaDataManager()->title(url):QString());
            duration = (available?QString::number(m_player->metaDataManager()->duration(url) / 1000):QString("-1"));

            if (type == PLS)
            {
                out << "File" << QString::number(i + 1) << "=";
            }
            else
            {
                out << "#EXTINF:" << duration << "," << title << "\n";
            }

            out << url.pathOrUrl() << '\n';

            if (type == PLS)
            {
                out << "Title" << QString::number(i + 1) << "=" << title << '\n';
                out << "Length" << QString::number(i + 1) << "=" << duration << "\n\n";
            }
        }

        if (type == PLS)
        {
            out << "Version=2";
        }

        data.close();

        KMessageBox::information(NULL, i18n("File saved successfully."));
    }
    else
    {
        KMessageBox::error(NULL, i18n("Cannot open file for writing."));
    }
}

void PlaylistManager::newPlaylist()
{
    createPlaylist(KInputDialog::getText(i18n("New playlist"), i18n("Enter name:")));
}

void PlaylistManager::clearPlaylist()
{
    m_playlists[visiblePlaylist()]->clear();
}

void PlaylistManager::shufflePlaylist()
{
    m_playlists[visiblePlaylist()]->shuffle();
}

void PlaylistManager::setCurrentPlaylist(int position)
{
    if (position > m_playlists.count())
    {
        position = 0;
    }

    m_currentPlaylist = position;

    if (m_dialog)
    {
        for (int i = 0; i < m_playlistUi.tabBar->count(); ++i)
        {
            m_playlistUi.tabBar->setTabIcon(i, KIcon((position == i)?"media-playback-start":"view-media-playlist"));
        }
    }

    m_player->setPlaylist(m_playlists[m_currentPlaylist]);

    emit configNeedsSaving();
}

void PlaylistManager::setDialogSize(const QSize &size)
{
    if (m_dialog)
    {
        m_dialog->resize(size);
    }
}

void PlaylistManager::setSplitterState(const QByteArray &state)
{
    if (m_dialog)
    {
        m_playlistUi.splitter->restoreState(state);
    }
}

void PlaylistManager::setHeaderState(const QByteArray &state)
{
    if (m_dialog)
    {
        m_playlistUi.playlistView->horizontalHeader()->restoreState(state);
        m_playlistUi.playlistView->horizontalHeader()->resizeSection(1, 300);
    }
}

void PlaylistManager::updateTheme()
{
//     QPalette palette = m_dialog->palette();
//     palette.setColor(QPalette::WindowText, Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor));
//     palette.setColor(QPalette::ButtonText, Plasma::Theme::defaultTheme()->color(Plasma::Theme::ButtonTextColor));
//     palette.setColor(QPalette::Background, Plasma::Theme::defaultTheme()->color(Plasma::Theme::BackgroundColor));
//     palette.setColor(QPalette::Button, palette.color(QPalette::Background).lighter(250));
//
//     m_dialog->setPalette(palette);
}

QList<PlaylistModel*> PlaylistManager::playlists() const
{
    return m_playlists;
}

QSize PlaylistManager::dialogSize() const
{
    return (m_dialog?m_dialog->size():QSize());
}

QByteArray PlaylistManager::splitterState() const
{
    return (m_dialog?m_playlistUi.splitter->saveState():QByteArray());
}

QByteArray PlaylistManager::headerState() const
{
    return (m_dialog?m_playlistUi.playlistView->horizontalHeader()->saveState():QByteArray());
}

int PlaylistManager::currentPlaylist() const
{
    return m_currentPlaylist;
}

int PlaylistManager::visiblePlaylist() const
{
    int index = (m_dialog?m_playlistUi.tabBar->currentIndex():m_currentPlaylist);

    return ((index > m_playlists.count() || index < 0)?0:index);
}

bool PlaylistManager::isDialogVisible() const
{
    return (m_dialog && m_dialog->isVisible());
}

bool PlaylistManager::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::KeyPress && !m_editorActive)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_Return && m_playlistUi.playlistView->selectionModel()->selectedIndexes().count())
        {
            playTrack(m_playlistUi.playlistView->selectionModel()->selectedIndexes().first());
        }
        else
        {
            QCoreApplication::sendEvent(m_player->parent(), keyEvent);
        }

        return true;
    }
    else if (event->type() == QEvent::ContextMenu)
    {
        QPoint point = static_cast<QContextMenuEvent*>(event)->pos();
        point.setY(point.y() - m_playlistUi.playlistView->horizontalHeader()->height());

        QModelIndex index = m_playlistUi.playlistView->indexAt(point);

        if (index.data(Qt::DisplayRole).toString().isEmpty())
        {
            return false;
        }

        KUrl url = KUrl(index.data(Qt::ToolTipRole).toString());
        KMenu menu;
        menu.addAction(KIcon("document-edit"), i18n("Edit title"), this, SLOT(editTrackTitle()));
        menu.addAction(KIcon("edit-copy"), i18n("Copy URL"), this, SLOT(copyTrackUrl()));
        menu.addSeparator();

///FIXME use index
        if (url == m_player->url())
        {
            menu.addAction(m_player->action(PlayPauseAction));
            menu.addAction(m_player->action(StopAction));
        }
        else
        {
            menu.addAction(KIcon("media-playback-start"), i18n("Play"), this, SLOT(playTrack()));
        }

        menu.addSeparator();
        menu.addAction(KIcon("list-remove"), i18n("Remove"), this, SLOT(removeTrack()));
        menu.exec(QCursor::pos());

        return true;
    }

    return QObject::eventFilter(object, event);
}

}
