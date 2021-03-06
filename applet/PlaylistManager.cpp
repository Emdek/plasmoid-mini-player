/***********************************************************************************
* Mini Player: Advanced media player for Plasma.
* Copyright (C) 2008 - 2013 Michal Dutkiewicz aka Emdek <emdeck@gmail.com>
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
#include "PlaylistWriter.h"
#include "MetaDataManager.h"
#include "Player.h"
#include "VideoWidget.h"

#include <QtGui/QKeyEvent>
#include <QtGui/QClipboard>
#include <QtGui/QHeaderView>
#include <QtGui/QContextMenuEvent>

#include <KMenu>
#include <KMessageBox>
#include <KFileDialog>
#include <KInputDialog>

#include <Plasma/Theme>

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
    m_selectedPlaylist(-1),
    m_removeTracks(0),
    m_splitterLocked(true),
    m_isEdited(false)
{
    m_columnsOrder << "fileType" << "fileName" << "artist" << "title" << "album" << "trackNumber" << "genre" << "description" << "date" << "duration";
    m_columnsVisibility << "fileType" << "artist" << "title" << "duration";
    m_columns[FileTypeColumn] = "fileType";
    m_columns[FileNameColumn] = "fileName";
    m_columns[ArtistColumn] = "artist";
    m_columns[TitleColumn] = "title";
    m_columns[AlbumColumn] = "album";
    m_columns[TrackNumberColumn] = "trackNumber";
    m_columns[GenreColumn] = "genre";
    m_columns[DescriptionColumn] = "description";
    m_columns[DateColumn] = "date";
    m_columns[DurationColumn] = "duration";

    m_videoWidget->hide();

    m_player->registerDialogVideoWidget(m_videoWidget);

    foreach (Solid::Device device, Solid::Device::listFromType(Solid::DeviceInterface::OpticalDisc, QString()))
    {
        deviceAdded(device.udi());
    }

    connect(m_player, SIGNAL(requestDevicePlaylist(QString,KUrl::List)), this, SLOT(createDevicePlaylist(QString,KUrl::List)));
    connect(m_player->action(OpenMenuAction)->menu(), SIGNAL(triggered(QAction*)), this, SLOT(openDisc(QAction*)));
    connect(m_player->action(PlaybackModeMenuAction)->menu(), SIGNAL(triggered(QAction*)), this, SLOT(playbackModeChanged(QAction*)));
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(QString)), this, SLOT(deviceAdded(QString)));
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(QString)), this, SLOT(deviceRemoved(QString)));
}

void PlaylistManager::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_removeTracks)
    {
        m_removeTracks = 0;

        QSet<KUrl> removedTracks = m_removedTracks;

        m_removedTracks.clear();

        QSet<KUrl> existingTracks;
        QMap<int, PlaylistModel*>::iterator iterator;

        for (iterator = m_playlists.begin(); iterator != m_playlists.end(); ++iterator)
        {
            if (iterator.value()->isReadOnly())
            {
                continue;
            }

            existingTracks = existingTracks.unite(iterator.value()->tracks().toSet());
        }

        removedTracks = removedTracks.subtract(existingTracks);

        QSet<KUrl>::iterator i;

        for (i = removedTracks.begin(); i != removedTracks.end(); ++i)
        {
            MetaDataManager::removeMetaData(*i);
        }
    }

    killTimer(event->timerId());
}

void PlaylistManager::columnsOrderChanged()
{
    if (!m_dialog)
    {
        return;
    }

    QStringList order;

    for (int i = 0; i < m_playlistUi.playlistView->horizontalHeader()->count(); ++i)
    {
        order.append(m_columns[static_cast<PlaylistColumn>(m_playlistUi.playlistView->horizontalHeader()->visualIndex(i))]);
    }

    m_columnsOrder = order;

    emit modified();
}

void PlaylistManager::visiblePlaylistChanged(int position)
{
    if (!m_dialog || position < 0 || position >= m_playlistsOrder.count())
    {
        return;
    }

    if (m_player->state() == StoppedState)
    {
        setCurrentPlaylist(m_playlistsOrder[position]);
    }

    if (m_playlistUi.playlistView->model())
    {
        disconnect(m_playlistUi.playlistView->model(), SIGNAL(modified()), this, SLOT(filterPlaylist()));
    }

    PlaylistModel *playlist = m_playlists[m_playlistsOrder[position]];

    m_playlistUi.playlistView->setModel(playlist);
    m_playlistUi.playlistView->horizontalHeader()->setMovable(true);
    m_playlistUi.playlistView->horizontalHeader()->setResizeMode(0, QHeaderView::Fixed);
    m_playlistUi.playlistView->horizontalHeader()->resizeSection(0, 22);

    connect(playlist, SIGNAL(modified()), this, SLOT(filterPlaylist()));

    filterPlaylist(m_playlistUi.playlistViewFilter->text());

    updateActions();

    m_playlistUi.playlistView->scrollTo(playlist->index(playlist->currentTrack(), 0), QAbstractItemView::PositionAtCenter);

    emit modified();
}

void PlaylistManager::playbackModeChanged(QAction *action)
{
    m_playlists[visiblePlaylist()]->setPlaybackMode(static_cast<PlaybackMode>(action->data().toInt()));
}

void PlaylistManager::toggleColumnVisibility(QAction *action)
{
    QStringList visibility;

    m_playlistUi.playlistView->horizontalHeader()->setSectionHidden(action->data().toInt(), !m_playlistUi.playlistView->horizontalHeader()->isSectionHidden(action->data().toInt()));

    for (int i = 0; i < m_playlistUi.playlistView->horizontalHeader()->count(); ++i)
    {
        if (!m_playlistUi.playlistView->horizontalHeader()->isSectionHidden(i))
        {
            visibility.append(m_columns[static_cast<PlaylistColumn>(i)]);
        }
    }

    setColumnsVisibility(visibility);
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
    const Solid::Device device = Solid::Device(udi);

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

    QMap<QString, QVariant> description;
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
    m_playlistsOrder.swap(from, to);

    emit modified();
}

void PlaylistManager::filterPlaylist()
{
    if (!m_playlistUi.playlistViewFilter->text().isEmpty())
    {
        filterPlaylist(m_playlistUi.playlistViewFilter->text());
    }
}

void PlaylistManager::filterPlaylist(const QString &text)
{
    PlaylistModel *playlist = m_playlists[visiblePlaylist()];
    QList<int> visibleSections;

    for (int i = 1; i < m_playlistUi.playlistView->horizontalHeader()->count(); ++i)
    {
        if (!m_playlistUi.playlistView->horizontalHeader()->isSectionHidden(i))
        {
            visibleSections.append(i);
        }
    }

    for (int i = 0; i < playlist->trackCount(); ++i)
    {
        bool hide = true;

        if (text.isEmpty())
        {
            hide = false;
        }
        else
        {
            for (int j = 0; j < visibleSections.count(); ++j)
            {
                if (playlist->index(i, visibleSections.at(j)).data(Qt::DisplayRole).toString().contains(text, Qt::CaseInsensitive))
                {
                    hide = false;

                    break;
                }
            }
        }

        m_playlistUi.playlistView->setRowHidden(i, hide);
    }
}

void PlaylistManager::renamePlaylist(int position)
{
    if (position >= m_playlists.count())
    {
        return;
    }

    if (position < 0)
    {
        position = m_selectedPlaylist;
    }

    PlaylistModel *playlist = m_playlists[m_playlistsOrder[position]];
    const QString title = KInputDialog::getText(i18n("Rename Playlist"), i18n("Enter name:"), playlist->title());

    if (title.isEmpty())
    {
        return;
    }

    m_playlistUi.tabBar->setTabText(position, title);

    playlist->setTitle(title);

    emit playlistChanged(position);
    emit modified();
}

void PlaylistManager::removePlaylist(int position)
{
    if (position >= m_playlists.count())
    {
        return;
    }

    if (position < 0)
    {
        position = m_selectedPlaylist;
    }

    if (m_playlistUi.tabBar->count() == 1)
    {
        clearPlaylist();

        PlaylistModel *playlist = m_playlists[visiblePlaylist()];
        playlist->setTitle(i18n("Default"));
        playlist->setCreationDate(QDateTime::currentDateTime());
        playlist->setModificationDate(QDateTime::currentDateTime());
        playlist->setLastPlayedDate(QDateTime());

        m_playlistUi.tabBar->setTabText(0, i18n("Default"));

        return;
    }

    m_playlists[m_playlistsOrder[position]]->deleteLater();

    m_playlistsOrder.removeAt(position);

    m_playlistUi.tabBar->removeTab(position);

    if (m_playlistUi.tabBar->count() == 1)
    {
        m_playlistUi.tabBar->hide();
    }

    if (currentPlaylist() == position)
    {
        m_player->stop();

        visiblePlaylistChanged((position == 0)?0:(position - 1));
    }

    emit playlistRemoved(position);
    emit modified();
}

void PlaylistManager::exportPlaylist()
{
    PlaylistModel *playlist = m_playlists[visiblePlaylist()];
    KFileDialog dialog(KUrl("~"), QString(), NULL);
    dialog.setMimeFilter(QStringList() << "audio/x-scpls" << "audio/x-mpegurl" << "application/xspf+xml" << "video/x-ms-asf");
    dialog.setWindowModality(Qt::NonModal);
    dialog.setMode(KFile::File);
    dialog.setOperationMode(KFileDialog::Saving);
    dialog.setConfirmOverwrite(true);
    dialog.setSelection(playlist->title() + ".pls");
    dialog.exec();

    if (dialog.selectedUrl().isEmpty())
    {
        return;
    }

    PlaylistWriter writer(this, dialog.selectedUrl().toLocalFile(), playlist);

    if (!writer.status())
    {
        KMessageBox::error(NULL, i18n("Cannot save playlist."));
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
    if (visiblePlaylist() == currentPlaylist())
    {
        m_player->stop();
    }

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
    PlaylistModel *playlist = m_playlists[visiblePlaylist()];
    const KUrl url(m_playlistUi.playlistView->currentIndex().data(Qt::UserRole).toUrl());
    const int row = m_playlistUi.playlistView->currentIndex().row();

    playlist->removeTrack(row);
    playlist->addTrack((row - 1), url);

    m_playlistUi.playlistView->setCurrentIndex(playlist->index((row - 1), 0));

    updateActions();
}

void PlaylistManager::moveDownTrack()
{
    PlaylistModel *playlist = m_playlists[visiblePlaylist()];
    const KUrl url(m_playlistUi.playlistView->currentIndex().data(Qt::UserRole).toUrl());
    const int row = m_playlistUi.playlistView->currentIndex().row();

    playlist->removeTrack(row);
    playlist->addTrack((row + 1), url);

    m_playlistUi.playlistView->setCurrentIndex(playlist->index((row + 1), 0));

    updateActions();
}

void PlaylistManager::removeTrack()
{
    PlaylistModel *playlist = m_playlists[visiblePlaylist()];
    const int row = m_playlistUi.playlistView->currentIndex().row();

    playlist->removeTrack(row);

    m_playlistUi.playlistView->setCurrentIndex(playlist->index(row, 0));
}

void PlaylistManager::playTrack(QModelIndex index)
{
    if (!index.isValid())
    {
        index = m_playlistUi.playlistView->currentIndex();
    }

    if (visiblePlaylist() != currentPlaylist())
    {
        setCurrentPlaylist(visiblePlaylist());
    }

    PlaylistModel *playlist = m_playlists[visiblePlaylist()];

    if (m_player->playlist() == playlist && index.row() == playlist->currentTrack() && m_player->state() != StoppedState)
    {
        m_player->playPause();
    }
    else
    {
        m_player->play(index.row());
    }
}

void PlaylistManager::editTrack(QAction *action)
{
    if (action && action->data().toInt() > 0)
    {
        m_isEdited = true;

        m_playlistUi.playlistView->edit(m_playlistUi.playlistView->model()->index(m_playlistUi.playlistView->currentIndex().row(), action->data().toInt()));
    }
    else if (m_playlistUi.playlistView->currentIndex().row() >= 0)
    {
        const KUrl url(m_playlists[visiblePlaylist()]->tracks().at(m_playlistUi.playlistView->currentIndex().row()));
        QWidget *trackWidget = new QWidget;

        m_trackUi.setupUi(trackWidget);
        m_trackUi.pathLineEdit->setText(url.pathOrUrl());
        m_trackUi.artistLineEdit->setText(MetaDataManager::metaData(url, ArtistKey, false));
        m_trackUi.titleLineEdit->setText(MetaDataManager::metaData(url, TitleKey, false));
        m_trackUi.albumLineEdit->setText(MetaDataManager::metaData(url, AlbumKey, false));
        m_trackUi.genreLineEdit->setText(MetaDataManager::metaData(url, GenreKey, false));
        m_trackUi.descriptionLineEdit->setText(MetaDataManager::metaData(url, DescriptionKey, false));
        m_trackUi.trackNumberSpinBox->setValue(MetaDataManager::metaData(url, TrackNumberKey, false).toInt());
        m_trackUi.yearSpinBox->setValue(MetaDataManager::metaData(url, DateKey, false).toInt());

        KDialog *trackDialog = new KDialog;
        trackDialog->setMainWidget(trackWidget);
        trackDialog->setButtons(KDialog::Cancel | KDialog::Ok);

        connect(trackDialog, SIGNAL(okClicked()), this, SLOT(saveTrack()));
        connect(trackDialog, SIGNAL(finished()), trackDialog, SLOT(deleteLater()));

        trackDialog->setWindowTitle(QFileInfo(url.pathOrUrl()).fileName());
        trackDialog->show();
    }
}

void PlaylistManager::copyTrack(QAction *action)
{
    PlaylistModel *sourcePlaylist = m_playlists[currentPlaylist()];

    if (!sourcePlaylist)
    {
        return;
    }

    KUrl::List urls;
    const QModelIndexList selectedRows = m_playlistUi.playlistView->selectionModel()->selectedRows();

    for (int i = 0; i < selectedRows.count(); ++i)
    {
        urls.append(sourcePlaylist->track(selectedRows.at(i).row()));
    }

    int target = action->data().toInt();

    if (target < 0 || !m_playlists.contains(target))
    {
        const QString title = KInputDialog::getText(i18n("New playlist"), i18n("Enter name:"));

        if (title.isEmpty())
        {
            return;
        }

        target = createPlaylist(title);
    }

    PlaylistModel *targetPlaylist = m_playlists[target];

    if (!targetPlaylist)
    {
        return;
    }

    targetPlaylist->addTracks(urls);
}

void PlaylistManager::saveTrack()
{
    const KUrl url(m_trackUi.pathLineEdit->text());

    MetaDataManager::setMetaData(url, ArtistKey, m_trackUi.artistLineEdit->text());
    MetaDataManager::setMetaData(url, TitleKey, m_trackUi.titleLineEdit->text());
    MetaDataManager::setMetaData(url, AlbumKey, m_trackUi.albumLineEdit->text());
    MetaDataManager::setMetaData(url, GenreKey, m_trackUi.genreLineEdit->text());
    MetaDataManager::setMetaData(url, DescriptionKey, m_trackUi.descriptionLineEdit->text());
    MetaDataManager::setMetaData(url, TrackNumberKey, QString::number(m_trackUi.trackNumberSpinBox->value()));
    MetaDataManager::setMetaData(url, DateKey, QString::number(m_trackUi.yearSpinBox->value()));
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

    PlaylistModel *playlist = m_playlists[visiblePlaylist()];

    m_player->action(PlaybackModeMenuAction)->menu()->actions().at(static_cast<int>(playlist->playbackMode()))->setChecked(true);

    QModelIndexList selectedIndexes = m_playlistUi.playlistView->selectionModel()->selectedIndexes();
    bool hasTracks = playlist->trackCount();

    m_playlistUi.addButton->setEnabled(!playlist->isReadOnly());
    m_playlistUi.removeButton->setEnabled(!selectedIndexes.isEmpty());
    m_playlistUi.editButton->setEnabled(!selectedIndexes.isEmpty() && !playlist->isReadOnly());
    m_playlistUi.moveUpButton->setEnabled((playlist->trackCount() > 1) && !selectedIndexes.isEmpty() && selectedIndexes.first().row() != 0);
    m_playlistUi.moveDownButton->setEnabled((playlist->trackCount() > 1) && !selectedIndexes.isEmpty() && selectedIndexes.last().row() != (playlist->trackCount() - 1));
    m_playlistUi.clearButton->setEnabled(hasTracks && !playlist->isReadOnly());
    m_playlistUi.playbackModeButton->setEnabled(hasTracks);
    m_playlistUi.exportButton->setEnabled(hasTracks && !playlist->isReadOnly());
    m_playlistUi.shuffleButton->setEnabled((playlist->trackCount() > 1) && !playlist->isReadOnly());
    m_playlistUi.playlistViewFilter->setEnabled(hasTracks);

    m_isEdited = false;
}

void PlaylistManager::updateTheme()
{
    if (!m_dialog)
    {
        return;
    }

    QPalette palette = m_dialog->palette();
    palette.setColor(QPalette::WindowText, Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor));
    palette.setColor(QPalette::ButtonText, Plasma::Theme::defaultTheme()->color(Plasma::Theme::ButtonTextColor));
    palette.setColor(QPalette::Background, Plasma::Theme::defaultTheme()->color(Plasma::Theme::BackgroundColor));
    palette.setColor(QPalette::Button, palette.color(QPalette::Background).lighter(250));

    m_dialog->setPalette(palette);
}

void PlaylistManager::updateLabel()
{
    if (!m_dialog)
    {
        return;
    }

    const QString artist = m_player->metaData(ArtistKey, false);
    const QString title = m_player->metaData(TitleKey, false);

    if (artist.isEmpty() || title.isEmpty())
    {
        m_playlistUi.titleLabel->setText(artist.isEmpty()?title:artist);
    }
    else
    {
        m_playlistUi.titleLabel->setText(QString("%1 - %2").arg(artist).arg(title));
    }
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

void PlaylistManager::removeTracks(const KUrl::List &tracks)
{
    if (m_removeTracks)
    {
        killTimer(m_removeTracks);
    }

    m_removedTracks.unite(tracks.toSet());

    m_removeTracks = startTimer(1000);
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
        m_playlistUi.tabBar->installEventFilter(this);
        m_playlistUi.playlistView->installEventFilter(this);
        m_playlistUi.playlistView->viewport()->installEventFilter(this);
        m_playlistUi.playlistView->horizontalHeader()->installEventFilter(this);
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
        m_playlistUi.playPreviousButton->setDefaultAction(m_player->action(PlayPreviousAction));
        m_playlistUi.playNextButton->setDefaultAction(m_player->action(PlayNextAction));
        m_playlistUi.seekSlider->setPlayer(m_player);
        m_playlistUi.muteButton->setDefaultAction(m_player->action(MuteAction));
        m_playlistUi.volumeSlider->setPlayer(m_player);
        m_playlistUi.fullScreenButton->setDefaultAction(m_player->action(FullScreenAction));
        m_playlistUi.titleLabel->setText(m_player->metaData(TitleKey));
        m_playlistUi.splitter->setStretchFactor(0, 1);
        m_playlistUi.splitter->setStretchFactor(1, 10);
        m_playlistUi.splitter->setStretchFactor(2, 1);
        m_playlistUi.splitter->setStretchFactor(3, 10);
        m_playlistUi.splitter->setStretchFactor(4, 1);
        m_playlistUi.splitter->setStretchFactor(5, 1);

        const int currentTab = m_playlistsOrder.indexOf(currentPlaylist());

        for (int i = 0; i < m_playlistsOrder.count(); ++i)
        {
            PlaylistModel *playlist = m_playlists[m_playlistsOrder[i]];

            m_playlistUi.tabBar->addTab(((currentTab == i)?KIcon("media-playback-start"):playlist->icon()), playlist->title());
        }

        m_playlistUi.tabBar->setVisible(m_playlists.count() > 1);
        m_playlistUi.tabBar->setCurrentIndex(currentTab);

        visiblePlaylistChanged(currentTab);
        setColumnsOrder(m_columnsOrder);
        setColumnsVisibility(m_columnsVisibility);
        setSplitterLocked(m_splitterLocked);
        setSplitterState(m_splitterState);
        setHeaderState(m_headerState);
        updateTheme();

        m_videoWidget->show();

        m_dialog->setContentsMargins(0, 0, 0, 0);
        m_dialog->adjustSize();
        m_dialog->resize(m_size);
        m_dialog->setMouseTracking(true);
        m_dialog->installEventFilter(this);
        m_dialog->installEventFilter(m_player->parent());

        connect(m_dialog, SIGNAL(dialogResized()), this, SIGNAL(modified()));
        connect(m_playlistUi.splitter, SIGNAL(splitterMoved(int,int)), this, SIGNAL(modified()));
        connect(m_playlistUi.tabBar, SIGNAL(newTabRequest()), this, SLOT(newPlaylist()));
        connect(m_playlistUi.tabBar, SIGNAL(tabDoubleClicked(int)), this, SLOT(renamePlaylist(int)));
        connect(m_playlistUi.tabBar, SIGNAL(closeRequest(int)), this, SLOT(removePlaylist(int)));
        connect(m_playlistUi.tabBar, SIGNAL(currentChanged(int)), this, SLOT(visiblePlaylistChanged(int)));
        connect(m_playlistUi.tabBar, SIGNAL(tabMoved(int,int)), this, SLOT(playlistMoved(int,int)));
        connect(m_playlistUi.closeButton, SIGNAL(clicked()), m_dialog, SLOT(close()));
        connect(m_playlistUi.addButton, SIGNAL(clicked()), m_player->action(OpenFileAction), SLOT(trigger()));
        connect(m_playlistUi.removeButton, SIGNAL(clicked()), this, SLOT(removeTrack()));
        connect(m_playlistUi.editButton, SIGNAL(clicked()), this, SLOT(editTrack()));
        connect(m_playlistUi.moveUpButton, SIGNAL(clicked()), this, SLOT(moveUpTrack()));
        connect(m_playlistUi.moveDownButton, SIGNAL(clicked()), this, SLOT(moveDownTrack()));
        connect(m_playlistUi.newButton, SIGNAL(clicked()), this, SLOT(newPlaylist()));
        connect(m_playlistUi.exportButton, SIGNAL(clicked()), this, SLOT(exportPlaylist()));
        connect(m_playlistUi.clearButton, SIGNAL(clicked()), this, SLOT(clearPlaylist()));
        connect(m_playlistUi.shuffleButton, SIGNAL(clicked()), this, SLOT(shufflePlaylist()));
        connect(m_playlistUi.playlistView, SIGNAL(pressed(QModelIndex)), this, SLOT(updateActions()));
        connect(m_playlistUi.playlistView, SIGNAL(activated(QModelIndex)), this, SLOT(playTrack(QModelIndex)));
        connect(m_playlistUi.playlistView->horizontalHeader(), SIGNAL(sectionMoved(int,int,int)), this, SLOT(columnsOrderChanged()));
        connect(m_playlistUi.playlistViewFilter, SIGNAL(textChanged(QString)), this, SLOT(filterPlaylist(QString)));
        connect(m_player, SIGNAL(metaDataChanged()), this, SLOT(updateLabel()));
        connect(m_player, SIGNAL(currentTrackChanged()), this, SLOT(updateLabel()));
        connect(m_player->action(FullScreenAction), SIGNAL(triggered()), this, SLOT(updateVideoView()));
        connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(updateTheme()));
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

void PlaylistManager::setCurrentPlaylist(int id)
{
    if (!m_playlists.contains(id))
    {
        id = visiblePlaylist();
    }

    m_player->setPlaylist(m_playlists[id]);

    if (m_dialog)
    {
        const int currentTab = m_playlistsOrder.indexOf(currentPlaylist());

        for (int i = 0; i < m_playlistUi.tabBar->count(); ++i)
        {
            m_playlistUi.tabBar->setTabIcon(i, ((currentTab == i)?KIcon("media-playback-start"):m_playlists[m_playlistsOrder[i]]->icon()));
        }
    }

    emit currentPlaylistChanged(id);
    emit modified();
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

void PlaylistManager::setPlaylistsOrder(const QList<int> &order)
{
    if (!m_dialog)
    {
        m_playlistsOrder = order;
    }
}

void PlaylistManager::setColumnsOrder(const QStringList &order)
{
    m_columnsOrder = order;

    emit modified();

    if (!m_dialog)
    {
        return;
    }

    for (int i = 0; i < m_playlistUi.playlistView->horizontalHeader()->count(); ++i)
    {
        m_playlistUi.playlistView->horizontalHeader()->moveSection(m_playlistUi.playlistView->horizontalHeader()->visualIndex(m_columns.key(order.value(i, QString()), static_cast<PlaylistColumn>(i))), i);
    }
}

void PlaylistManager::setColumnsVisibility(const QStringList &visibility)
{
    m_columnsVisibility = visibility;

    emit modified();

    if (!m_dialog)
    {
        return;
    }

    for (int i = 0; i < m_playlistUi.playlistView->horizontalHeader()->count(); ++i)
    {
        m_playlistUi.playlistView->horizontalHeader()->setSectionHidden(i, !m_columnsVisibility.contains(m_columns[static_cast<PlaylistColumn>(i)]));
    }
}

void PlaylistManager::setSplitterLocked(bool locked)
{
    m_splitterLocked = locked;

    emit modified();

    if (!m_dialog)
    {
        return;
    }

    m_playlistUi.splitter->setHandleWidth(locked?0:5);

    for (int i = 0; i < m_playlistUi.splitter->count(); ++i)
    {
        if (m_playlistUi.splitter->handle(i))
        {
            m_playlistUi.splitter->handle(i)->setCursor(locked?Qt::ArrowCursor:Qt::SplitVCursor);
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
        m_playlistUi.playlistView->horizontalHeader()->resizeSection(ArtistColumn, 200);
        m_playlistUi.playlistView->horizontalHeader()->resizeSection(TitleColumn, 250);
        m_playlistUi.playlistView->horizontalHeader()->resizeSection(DurationColumn, 80);
        m_playlistUi.playlistView->horizontalHeader()->restoreState(state);
    }
    else
    {
        m_headerState = state;
    }
}

PlaylistModel* PlaylistManager::playlist(int id) const
{
    return m_playlists.value(id, NULL);
}

QList<int> PlaylistManager::playlists() const
{
    return m_playlistsOrder;
}

QStringList PlaylistManager::columnsOrder() const
{
    return m_columnsOrder;
}

QStringList PlaylistManager::columnsVisibility() const
{
    return m_columnsVisibility;
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

int PlaylistManager::createPlaylist(const QString &title, const KUrl::List &tracks, PlaylistSource source, int id)
{
    if (id < 0 || m_playlists.contains(id))
    {
        id = 0;

        while (m_playlists.contains(id))
        {
            ++id;
        }
    }

    m_playlists[id] = new PlaylistModel(this, id, title, source);

    int position = qMin(m_playlists.count(), (m_playlistsOrder.indexOf(visiblePlaylist()) + 1));

    m_playlistsOrder.insert(position, id);

    if (!tracks.isEmpty())
    {
        m_playlists[id]->addTracks(tracks);
        m_playlists[id]->setCurrentTrack(0);
    }

    if (m_dialog)
    {
        m_playlistUi.tabBar->show();
        m_playlistUi.tabBar->insertTab(position, m_playlists[id]->icon(), title);
        m_playlistUi.tabBar->setCurrentIndex(position);

        visiblePlaylistChanged(position);
    }


    emit playlistAdded(position);
    emit modified();

    connect(m_playlists[id], SIGNAL(modified()), this, SIGNAL(modified()));
    connect(m_playlists[id], SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(trackChanged()));

    return id;
}

int PlaylistManager::currentPlaylist() const
{
    const int id = m_playlists.key(m_player->playlist(), -1);

    return ((id < 0)?visiblePlaylist():id);
}

int PlaylistManager::visiblePlaylist() const
{
    if (m_dialog && m_playlistUi.tabBar->currentIndex() >= 0 && m_playlistUi.tabBar->currentIndex() < m_playlistsOrder.count())
    {
        return m_playlistsOrder[m_playlistUi.tabBar->currentIndex()];
    }

    const int id = m_playlists.key(m_player->playlist(), -1);

    return ((id < 0)?m_playlistsOrder.first():id);
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

            if ((keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Delete) && m_playlistUi.playlistView->selectionModel()->selectedIndexes().count())
            {
                if (keyEvent->key() == Qt::Key_Return)
                {
                    playTrack();
                }
                else
                {
                    removeTrack();
                }
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
                PlaylistModel *playlist = m_playlists[visiblePlaylist()];
                KMenu menu;

                if (m_player->playlist() == playlist && index.row() == playlist->currentTrack() && m_player->state() == PlayingState)
                {
                    menu.addAction(m_player->action(PlayPauseAction));
                    menu.addAction(m_player->action(StopAction));
                }
                else
                {
                    menu.addAction(KIcon("media-playback-start"), i18n("Play"), this, SLOT(playTrack()));
                }

                menu.addSeparator();

                QMenu *editMenu = menu.addMenu(KIcon("document-edit"), i18n("Edit"));
                editMenu->setEnabled(!playlist->isReadOnly());
                editMenu->addAction(i18n("Edit all properties..."));
                editMenu->addSeparator();

                for (int i = 0; i < m_playlistUi.playlistView->horizontalHeader()->count(); ++i)
                {
                    const int index = m_playlistUi.playlistView->horizontalHeader()->visualIndex(i);

                    if (m_playlistUi.playlistView->horizontalHeader()->isSectionHidden(index) || index < ArtistColumn || index > DateColumn)
                    {
                        continue;
                    }

                    editMenu->addAction(i18n("Edit field \"%1\"...").arg(m_playlistUi.playlistView->model()->headerData(index, Qt::Horizontal).toString(), Qt::EditRole))->setData(index);
                }

                connect(editMenu, SIGNAL(triggered(QAction*)), this, SLOT(editTrack(QAction*)));

                menu.addAction(KIcon("edit-copy"), i18n("Copy URL"), this, SLOT(copyTrackUrl()));

                QMenu *addMenu = menu.addMenu(KIcon("list-add"), i18n("Add to playlist"));

                if (m_playlists.count() > 1)
                {
                    for (int i = 0; i < m_playlistsOrder.count(); ++i)
                    {
                        PlaylistModel *targetPlaylist = m_playlists[m_playlistsOrder.at(i)];

                        if (targetPlaylist && !targetPlaylist->isReadOnly() && targetPlaylist != playlist)
                        {
                            QAction *action = addMenu->addAction(targetPlaylist->icon(), targetPlaylist->title());
                            action->setData(targetPlaylist->id());
                        }
                    }

                    addMenu->addSeparator();
                }

                QAction *action = addMenu->addAction(KIcon("document-new"), i18n("New playlist..."));
                action->setData(-1);

                connect(addMenu, SIGNAL(triggered(QAction*)), this, SLOT(copyTrack(QAction*)));

                menu.addSeparator();

                menu.addAction(KIcon("list-remove"), i18n("Remove"), this, SLOT(removeTrack()));
                menu.exec(QCursor::pos());

                return true;
            }
        }
    }
    else if (object == m_playlistUi.playlistView->viewport() && event->type() == QEvent::Drop)
    {
        m_playlistUi.playlistView->clearSelection();
    }
    else if (object == m_playlistUi.playlistView->horizontalHeader() && event->type() == QEvent::ContextMenu)
    {
        KMenu menu;

        for (int i = 0; i < m_playlistUi.playlistView->horizontalHeader()->count(); ++i)
        {
            const int index = m_playlistUi.playlistView->horizontalHeader()->visualIndex(i);

            menu.addAction((m_playlistUi.playlistView->horizontalHeader()->isSectionHidden(index)?i18n("Show column \"%1\""):i18n("Hide column \"%1\"")).arg(m_playlistUi.playlistView->model()->headerData(index, Qt::Horizontal).toString(), Qt::EditRole))->setData(index);
        }

        connect(&menu, SIGNAL(triggered(QAction*)), this, SLOT(toggleColumnVisibility(QAction*)));

        menu.exec(QCursor::pos());

        return true;
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
    else if (object == m_playlistUi.tabBar)
    {
        if (event->type() == QEvent::ContextMenu)
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
        else if (event->type() == QEvent::DragEnter)
        {
            QDragEnterEvent *dragEvent = static_cast<QDragEnterEvent*>(event);

            if (dragEvent->mimeData()->hasUrls())
            {
                dragEvent->accept();

                return true;
            }
        }
        else if (event->type() == QEvent::DragMove)
        {
            QDragMoveEvent *dragEvent = static_cast<QDragMoveEvent*>(event);

            if (dragEvent->mimeData()->hasUrls())
            {
                const int tab = m_playlistUi.tabBar->tabAt(dragEvent->pos());

                if (tab != visiblePlaylist())
                {
                    dragEvent->accept();

                    if (tab >= 0)
                    {
                        m_playlistUi.tabBar->setCurrentIndex(tab);
                    }

                    return true;
                }
            }
        }
        else if (event->type() == QEvent::Drop)
        {
            QDropEvent *dropEvent = static_cast<QDropEvent*>(event);

            if (dropEvent->mimeData()->hasUrls())
            {
                int tab = m_playlistUi.tabBar->tabAt(dropEvent->pos());

                if (tab < 0)
                {
                    const QString title = KInputDialog::getText(i18n("New playlist"), i18n("Enter name:"));

                    if (!title.isEmpty())
                    {
                        tab = createPlaylist(title);
                    }
                }

                if (tab >= 0 && m_playlists[m_playlistsOrder[tab]])
                {
                    m_playlists[m_playlistsOrder[tab]]->addTracks(KUrl::List(dropEvent->mimeData()->urls()));

                    return true;
                }
            }
        }
    }
    else if (object == m_dialog && event->type() == QEvent::ContextMenu && m_dialog && !m_isEdited)
    {
        if (m_dialog->childAt(static_cast<QContextMenuEvent*>(event)->pos()) != m_playlistUi.playlistViewFilter)
        {
            emit requestMenu(QCursor::pos());

            return true;
        }
    }

    return QObject::eventFilter(object, event);
}

}
