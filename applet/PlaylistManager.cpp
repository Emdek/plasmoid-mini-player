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
#include "VideoWidget.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtGui/QKeyEvent>
#include <QtGui/QClipboard>
#include <QtGui/QHeaderView>
#include <QtGui/QContextMenuEvent>

#include <KMenu>
#include <KMessageBox>
#include <KFileDialog>
#include <KInputDialog>

#include <Solid/Block>
#include <Solid/Device>
#include <Solid/OpticalDisc>
#include <Solid/DeviceNotifier>

namespace MiniPlayer
{

PlaylistManager::PlaylistManager(Player *parent) : QObject(parent),
    m_player(parent),
    m_dialog(NULL),
    m_videoWidget(new VideoWidget(qobject_cast<QGraphicsWidget*>(m_player->parent()))),
    m_size(QSize(600, 500)),
    m_currentPlaylist(0),
    m_selectedPlaylist(-1),
    m_splitterLocked(true),
    m_isEdited(false)
{
    m_videoWidget->hide();

    m_player->registerDialogVideoWidget(m_videoWidget);

    foreach (Solid::Device device, Solid::Device::listFromType(Solid::DeviceInterface::OpticalDisc, QString()))
    {
        deviceAdded(device.udi());
    }

    connect(m_player, SIGNAL(createDevicePlaylist(QString,KUrl::List)), this, SLOT(createDevicePlaylist(QString,KUrl::List)));
    connect(m_player->action(OpenMenuAction)->menu(), SIGNAL(triggered(QAction*)), this, SLOT(openDisc(QAction*)));
    connect(m_player->action(PlaybackModeMenuAction)->menu(), SIGNAL(triggered(QAction*)), this, SLOT(playbackModeChanged(QAction*)));
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(QString)), this, SLOT(deviceAdded(QString)));
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(QString)), this, SLOT(deviceRemoved(QString)));
}

void PlaylistManager::visiblePlaylistChanged(int position)
{
    if (position > m_playlists.count())
    {
        return;
    }

    if (m_player->state() == StoppedState)
    {
        setCurrentPlaylist(position);
    }

    filterPlaylist(m_playlistUi.playlistViewFilter->text());

    m_playlistUi.playlistView->setModel(m_playlists[visiblePlaylist()]);
    m_playlistUi.playlistView->horizontalHeader()->setMovable(true);
    m_playlistUi.playlistView->horizontalHeader()->setResizeMode(0, QHeaderView::Fixed);
    m_playlistUi.playlistView->horizontalHeader()->resizeSection(0, 22);

    updateActions();

    emit needsSaving();
}

void PlaylistManager::playbackModeChanged(QAction *action)
{
    m_playlists[visiblePlaylist()]->setPlaybackMode(static_cast<PlaybackMode>(action->data().toInt()));
}

void PlaylistManager::openDisc(QAction *action)
{
    const QString udi = action->data().toString();

    if (!m_discActions.contains(udi))
    {
        return;
    }

    m_player->openDisc(m_discActions[udi].second["device"].toString(), static_cast<PlaylistSource>(m_discActions[udi].second["source"].toInt()));
}

void PlaylistManager::deviceAdded(const QString &udi)
{
    Solid::Device device = Solid::Device(udi);

    if (!device.isDeviceInterface(Solid::DeviceInterface::OpticalDisc))
    {
        return;
    }

    const Solid::OpticalDisc *opticalDisc = device.as<const Solid::OpticalDisc>();
    QString title = opticalDisc->label().replace('_', ' ');
    QString label;
    KIcon icon;
    PlaylistSource source;

    if (title.isEmpty())
    {
        title = i18n("Disc %1", (m_discActions.count() + 1));
    }

    if (opticalDisc->availableContent() & Solid::OpticalDisc::VideoDvd)
    {
        icon = KIcon("media-optical-dvd");
        label = i18n("Open DVD: %1", title);
        source = DvdSource;
    }
    else if (opticalDisc->availableContent() & Solid::OpticalDisc::VideoCd || opticalDisc->availableContent() & Solid::OpticalDisc::SuperVideoCd)
    {
        icon = KIcon("media-optical");
        label = i18n("Open VCD: %1", title);
        source = VcdSource;
    }
    else if (opticalDisc->availableContent() & Solid::OpticalDisc::Audio)
    {
        icon = KIcon("media-optical-audio");
        label = i18n("Open Audio CD: %1", title);
        source = CdSource;
    }
    else
    {
        return;
    }

    QHash<QString, QVariant> description;
    description["title"] = title;
    description["source"] = source;
    description["udi"] = udi;
    description["device"] = device.as<const Solid::Block>()->device();
    description["playlist"] = -1;

    m_discActions[udi] = qMakePair(m_player->action(OpenMenuAction)->menu()->addAction(icon, label), description);
    m_discActions[udi].first->setData(udi);
}

void PlaylistManager::deviceRemoved(const QString &udi)
{
    if (!m_discActions.contains(udi))
    {
        return;
    }

    if (m_discActions[udi].second["playlist"].toInt() >= 0)
    {
        removePlaylist(m_discActions[udi].second["playlist"].toInt());
    }

    m_discActions[udi].first->deleteLater();
    m_discActions.remove(udi);
}

void PlaylistManager::createDevicePlaylist(const QString &udi, const KUrl::List &tracks)
{
    if (!m_discActions.contains(udi) || tracks.isEmpty() || m_discActions[udi].second["playlist"].toInt() >= 0)
    {
        return;
    }

    m_discActions[udi].second["playlist"] = createPlaylist(m_discActions[udi].second["title"].toString(), tracks, static_cast<PlaylistSource>(m_discActions[udi].second["source"].toInt()));

    setCurrentPlaylist(m_discActions[udi].second["playlist"].toInt());

    m_player->setPlaylist(m_playlists[m_discActions[udi].second["playlist"].toInt()]);
    m_player->play();
}

void PlaylistManager::playlistMoved(int from, int to)
{
    m_playlists.swap(from, to);

    emit needsSaving();
}

void PlaylistManager::filterPlaylist(const QString &text)
{
    for (int i = 0; i < m_playlists[visiblePlaylist()]->trackCount(); ++i)
    {
        m_playlistUi.playlistView->setRowHidden(i, (!text.isEmpty() && !m_playlists[visiblePlaylist()]->index(i, 1).data(Qt::DisplayRole).toString().contains(text, Qt::CaseInsensitive)));
    }
}

void PlaylistManager::renamePlaylist(int position)
{
    if (position < 0)
    {
        position = m_selectedPlaylist;
    }

    const QString title = KInputDialog::getText(i18n("Rename Playlist"), i18n("Enter name:"), m_playlists[position]->title());

    if (title.isEmpty())
    {
        return;
    }

    m_playlistUi.tabBar->setTabText(position, title);
    m_playlists[position]->setTitle(title);

    emit needsSaving();
}

void PlaylistManager::removePlaylist(int position)
{
    if (position < 0)
    {
        position = m_selectedPlaylist;
    }

    if (m_playlistUi.tabBar->count() == 1)
    {
        clearPlaylist();

        return;
    }

    m_playlistUi.tabBar->removeTab(position);

    m_playlists[position]->deleteLater();
    m_playlists.removeAt(position);

    if (m_currentPlaylist == position)
    {
        m_player->stop();

        setCurrentPlaylist(qMax(0, (position - 1)));
    }

    if (m_playlistUi.tabBar->count() == 1)
    {
        m_playlistUi.tabBar->hide();
    }

    emit needsSaving();
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
        const PlaylistFormat type = (dialog.selectedUrl().toLocalFile().endsWith(".pls")?PlsFormat:M3uFormat);

        if (type == PlsFormat)
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
            KUrl url = m_playlists[visiblePlaylist()]->track(i);
            QString title = MetaDataManager::title(url, false);
            QString duration = ((MetaDataManager::duration(url) > 0)?QString::number(MetaDataManager::duration(url) / 1000):QString("-1"));

            if (type == PlsFormat)
            {
                out << "File" << QString::number(i + 1) << "=";
            }
            else
            {
                out << "#EXTINF:" << duration << "," << title << "\n";
            }

            out << url.pathOrUrl() << '\n';

            if (type == PlsFormat)
            {
                out << "Title" << QString::number(i + 1) << "=" << title << '\n';
                out << "Length" << QString::number(i + 1) << "=" << duration << "\n\n";
            }
        }

        if (type == PlsFormat)
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
    const QString title = KInputDialog::getText(i18n("New playlist"), i18n("Enter name:"));

    if (!title.isEmpty())
    {
        createPlaylist(title);
    }
}

void PlaylistManager::clearPlaylist()
{
    m_playlists[visiblePlaylist()]->clear();
}

void PlaylistManager::shufflePlaylist()
{
    m_playlists[visiblePlaylist()]->shuffle();
}

void PlaylistManager::trackChanged()
{
    m_isEdited = false;
}

void PlaylistManager::moveUpTrack()
{
    KUrl url(m_playlistUi.playlistView->currentIndex().data(Qt::UserRole).toUrl());
    int row = m_playlistUi.playlistView->currentIndex().row();

    m_playlists[visiblePlaylist()]->removeTrack(row);
    m_playlists[visiblePlaylist()]->addTrack((row - 1), url);

    m_playlistUi.playlistView->setCurrentIndex(m_playlists[visiblePlaylist()]->index((row - 1), 0));

    updateActions();
}

void PlaylistManager::moveDownTrack()
{
    KUrl url(m_playlistUi.playlistView->currentIndex().data(Qt::UserRole).toUrl());
    int row = m_playlistUi.playlistView->currentIndex().row();

    m_playlists[visiblePlaylist()]->removeTrack(row);
    m_playlists[visiblePlaylist()]->addTrack((row + 1), url);

    m_playlistUi.playlistView->setCurrentIndex(m_playlists[visiblePlaylist()]->index((row + 1), 0));

    updateActions();
}

void PlaylistManager::removeTrack()
{
    int row = m_playlistUi.playlistView->currentIndex().row();

    m_playlists[visiblePlaylist()]->removeTrack(row);

    m_playlistUi.playlistView->setCurrentIndex(m_playlists[visiblePlaylist()]->index(row, 0));
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

    if (m_playlists[visiblePlaylist()] == m_player->playlist() && index.row() == m_playlists[visiblePlaylist()]->currentTrack() && m_player->state() != StoppedState)
    {
        m_player->playPause();
    }
    else
    {
        m_player->play(index.row());
    }
}

void PlaylistManager::editTrackTitle()
{
    m_isEdited = true;

    m_playlistUi.playlistView->edit(m_playlistUi.playlistView->model()->index(m_playlistUi.playlistView->currentIndex().row(), 1));
}

void PlaylistManager::editTrackArtist()
{
    m_isEdited = true;

    m_playlistUi.playlistView->edit(m_playlistUi.playlistView->model()->index(m_playlistUi.playlistView->currentIndex().row(), 2));
}

void PlaylistManager::copyTrackUrl()
{
    QApplication::clipboard()->setText(m_playlistUi.playlistView->currentIndex().data(Qt::UserRole).toUrl().toString());
}

void PlaylistManager::updateActions()
{
    if (!m_dialog || !m_playlistUi.playlistView->selectionModel())
    {
        return;
    }

    m_player->action(PlaybackModeMenuAction)->menu()->actions().at(static_cast<int>(m_playlists[visiblePlaylist()]->playbackMode()))->setChecked(true);

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

    m_isEdited = false;
}

void PlaylistManager::updateVideoView()
{
    m_videoWidget->resize(m_playlistUi.graphicsView->size());

    m_playlistUi.graphicsView->centerOn(m_videoWidget);
    m_playlistUi.graphicsView->scene()->setSceneRect(m_playlistUi.graphicsView->rect());
}

void PlaylistManager::addTracks(const KUrl::List &tracks, int index, PlayerReaction reaction)
{
    m_playlists[visiblePlaylist()]->addTracks(tracks, index, reaction);

    updateActions();
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

        m_playlistUi.graphicsView->setScene(new QGraphicsScene(this));
        m_playlistUi.graphicsView->scene()->addItem(m_videoWidget);
        m_playlistUi.graphicsView->installEventFilter(this);
        m_playlistUi.playlistView->installEventFilter(this);
        m_playlistUi.closeButton->setIcon(KIcon("window-close"));
        m_playlistUi.addButton->setIcon(KIcon("list-add"));
        m_playlistUi.addButton->setDelayedMenu(m_player->action(OpenMenuAction)->menu());
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
        m_playlistUi.titleLabel->setText(m_player->title());
        m_playlistUi.splitter->setStretchFactor(0, 1);
        m_playlistUi.splitter->setStretchFactor(1, 10);
        m_playlistUi.splitter->setStretchFactor(2, 1);
        m_playlistUi.splitter->setStretchFactor(3, 10);
        m_playlistUi.splitter->setStretchFactor(4, 1);
        m_playlistUi.splitter->setStretchFactor(5, 1);

        for (int i = 0; i < m_playlists.count(); ++i)
        {
            m_playlistUi.tabBar->addTab(((m_currentPlaylist == i)?KIcon("media-playback-start"):m_playlists.at(i)->icon()), m_playlists.at(i)->title());
        }

        m_playlistUi.tabBar->setVisible(m_playlists.count() > 1);

        visiblePlaylistChanged(m_currentPlaylist);
        setSplitterLocked(m_splitterLocked);
        setSplitterState(m_splitterState);
        setHeaderState(m_headerState);

        m_videoWidget->show();

        m_dialog->setContentsMargins(0, 0, 0, 0);
        m_dialog->adjustSize();
        m_dialog->resize(m_size);
        m_dialog->setMouseTracking(true);
        m_dialog->installEventFilter(this);
        m_dialog->installEventFilter(m_player->parent());

        connect(m_dialog, SIGNAL(dialogResized()), this, SIGNAL(needsSaving()));
        connect(m_playlistUi.splitter, SIGNAL(splitterMoved(int,int)), this, SIGNAL(needsSaving()));
        connect(m_playlistUi.tabBar, SIGNAL(newTabRequest()), this, SLOT(newPlaylist()));
        connect(m_playlistUi.tabBar, SIGNAL(tabDoubleClicked(int)), this, SLOT(renamePlaylist(int)));
        connect(m_playlistUi.tabBar, SIGNAL(closeRequest(int)), this, SLOT(removePlaylist(int)));
        connect(m_playlistUi.tabBar, SIGNAL(currentChanged(int)), this, SLOT(visiblePlaylistChanged(int)));
        connect(m_playlistUi.tabBar, SIGNAL(tabMoved(int,int)), this, SLOT(playlistMoved(int,int)));
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
        connect(m_playlistUi.playlistView, SIGNAL(pressed(QModelIndex)), this, SLOT(updateActions()));
        connect(m_playlistUi.playlistView, SIGNAL(activated(QModelIndex)), this, SLOT(playTrack(QModelIndex)));
        connect(m_playlistUi.playlistViewFilter, SIGNAL(textChanged(QString)), this, SLOT(filterPlaylist(QString)));
        connect(m_player->parent(), SIGNAL(titleChanged(QString)), m_playlistUi.titleLabel, SLOT(setText(QString)));
        connect(m_player->action(FullScreenAction), SIGNAL(triggered()), this, SLOT(updateVideoView()));
        connect(this, SIGNAL(destroyed()), m_dialog, SLOT(deleteLater()));
    }

    m_dialog->move(position);
    m_dialog->show();

    updateActions();
    updateVideoView();
}

void PlaylistManager::closeDialog()
{
    if (m_dialog)
    {
        m_dialog->close();
    }
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
            m_playlistUi.tabBar->setTabIcon(i, ((m_currentPlaylist == i)?KIcon("media-playback-start"):m_playlists.at(i)->icon()));
        }
    }

    m_player->setPlaylist(m_playlists[m_currentPlaylist]);

    emit needsSaving();
}

void PlaylistManager::setDialogSize(const QSize &size)
{
    if (m_dialog)
    {
        m_dialog->resize(size);
    }
    else
    {
        m_size = size;
    }
}

void PlaylistManager::setSplitterLocked(bool locked)
{
    m_splitterLocked = locked;

    emit needsSaving();

    if (!m_dialog)
    {
        return;
    }

    m_playlistUi.splitter->setHandleWidth(locked?0:5);

    for (int i = 0; i < m_playlistUi.splitter->count(); ++i)
    {
        if (m_playlistUi.splitter->handle(i))
        {
            m_playlistUi.splitter->handle(i)->setEnabled(!locked);
            m_playlistUi.splitter->handle(i)->setVisible(!locked);
        }
    }
}

void PlaylistManager::setSplitterState(const QByteArray &state)
{
    if (m_dialog)
    {
        m_playlistUi.splitter->restoreState(state);
    }
    else
    {
        m_splitterState = state;
    }
}

void PlaylistManager::setHeaderState(const QByteArray &state)
{
    if (m_dialog)
    {
        m_playlistUi.playlistView->horizontalHeader()->resizeSection(1, 250);
        m_playlistUi.playlistView->horizontalHeader()->resizeSection(2, 200);
        m_playlistUi.playlistView->horizontalHeader()->resizeSection(3, 80);
        m_playlistUi.playlistView->horizontalHeader()->restoreState(state);
    }
    else
    {
        m_headerState = state;
    }
}

QList<PlaylistModel*> PlaylistManager::playlists() const
{
    return m_playlists;
}

QSize PlaylistManager::dialogSize() const
{
    return (m_dialog?m_dialog->size():m_size);
}

QByteArray PlaylistManager::splitterState() const
{
    return (m_dialog?m_playlistUi.splitter->saveState():m_splitterState);
}

QByteArray PlaylistManager::headerState() const
{
    return (m_dialog?m_playlistUi.playlistView->horizontalHeader()->saveState():m_headerState);
}

PlayerState PlaylistManager::state() const
{
    return m_player->state();
}

int PlaylistManager::createPlaylist(const QString &title, const KUrl::List &tracks, PlaylistSource source)
{
    PlaylistModel *playlist = new PlaylistModel(this, title, source);
    int position = qMin(m_playlists.count(), visiblePlaylist() + 1);

    m_playlists.insert(position, playlist);

    if (!tracks.isEmpty())
    {
        m_playlists[position]->addTracks(tracks);
    }

    if (m_dialog)
    {
        m_playlistUi.tabBar->show();
        m_playlistUi.tabBar->insertTab((position + 1), playlist->icon(), title);
        m_playlistUi.tabBar->setCurrentIndex(position + 1);
    }

    playlist->setCurrentTrack(0);

    emit needsSaving();

    connect(playlist, SIGNAL(needsSaving()), this, SIGNAL(needsSaving()));
    connect(playlist, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(trackChanged()));

    return position;
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

bool PlaylistManager::isSplitterLocked() const
{
    return m_splitterLocked;
}

bool PlaylistManager::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_playlistUi.playlistView)
    {
        if (event->type() == QEvent::KeyPress && !m_isEdited)
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
        else if (event->type() == QEvent::ContextMenu && !m_isEdited)
        {
            QPoint point = static_cast<QContextMenuEvent*>(event)->pos();
            point.setY(point.y() - m_playlistUi.playlistView->horizontalHeader()->height());

            QModelIndex index = m_playlistUi.playlistView->indexAt(point);

            if (index.isValid())
            {
                KMenu menu;
                QMenu *editMenu = menu.addMenu(KIcon("document-edit"), i18n("Edit"));
                editMenu->addAction(i18n("Edit title..."), this, SLOT(editTrackTitle()));
                editMenu->addAction(i18n("Edit artist..."), this, SLOT(editTrackArtist()));

                menu.addAction(KIcon("edit-copy"), i18n("Copy URL"), this, SLOT(copyTrackUrl()));
                menu.addSeparator();

                if (m_playlists[visiblePlaylist()] == m_player->playlist() && index.row() == m_playlists[visiblePlaylist()]->currentTrack())
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
        }
    }
    else if (object == m_playlistUi.graphicsView)
    {
        if (event->type() == QEvent::Resize)
        {
            updateVideoView();
        }
        else if (event->type() == QEvent::GraphicsSceneWheel)
        {
            return false;
        }
    }
    else if (event->type() == QEvent::ContextMenu && m_dialog && !m_isEdited)
    {
        QPoint point = static_cast<QContextMenuEvent*>(event)->pos();

        if (m_dialog->childAt(point) == m_playlistUi.tabBar)
        {
            m_selectedPlaylist = m_playlistUi.tabBar->tabAt(m_playlistUi.tabBar->mapFromGlobal(static_cast<QContextMenuEvent*>(event)->globalPos()));

            KMenu menu;
            menu.addAction(KIcon("document-new"), i18n("New playlist..."), this, SLOT(newPlaylist()));

            if (m_selectedPlaylist >= 0)
            {
                menu.addSeparator();
                menu.addAction(KIcon("edit-rename"), i18n("Rename playlist..."), this, SLOT(renamePlaylist()));
                menu.addSeparator();
                menu.addAction(KIcon("document-close"), i18n("Close playlist"), this, SLOT(removePlaylist()));
            }

            menu.exec(QCursor::pos());

            return true;
        }
        else if (m_dialog->childAt(point) != m_playlistUi.playlistViewFilter)
        {
            emit requestMenu(QCursor::pos());

        return true;
        }
    }

    return QObject::eventFilter(object, event);
}

}
