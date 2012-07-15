/***********************************************************************************
* Mini Player: Advanced media player for Plasma.
* Copyright (C) 2008 - 2012 Michal Dutkiewicz aka Emdek <emdeck@gmail.com>
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

#include "Applet.h"
#include "Configuration.h"
#include "Player.h"
#include "VideoWidget.h"
#include "MetaDataManager.h"
#include "PlaylistManager.h"
#include "PlaylistModel.h"
#include "SeekSlider.h"
#include "DBusInterface.h"

#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QTimerEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QGraphicsLinearLayout>
#include <QtGui/QGraphicsSceneResizeEvent>
#include <QtGui/QGraphicsSceneDragDropEvent>

#include <KIcon>
#include <KMenu>
#include <KLocale>
#include <KFileDialog>
#include <KWindowInfo>
#include <KInputDialog>
#include <KWindowSystem>
#include <KConfigDialog>

#include <Plasma/Corona>
#include <Plasma/Slider>
#include <Plasma/ToolButton>
#include <Plasma/Containment>
#include <Plasma/ToolTipManager>

K_EXPORT_PLASMA_APPLET(miniplayer, MiniPlayer::Applet)

namespace MiniPlayer
{

Applet::Applet(QObject *parent, const QVariantList &args) : Plasma::Applet(parent, args),
    m_player(new Player(this)),
    m_playlistManager(new PlaylistManager(m_player)),
    m_dBusInterface(NULL),
    m_videoWidget(NULL),
    m_controlsWidget(NULL),
    m_volumeDialog(NULL),
    m_jumpToPositionDialog(NULL),
    m_togglePlaylist(0),
    m_hideToolTip(0),
    m_updateToolTip(0),
    m_initialized(false)
{
    KGlobal::locale()->insertCatalog("miniplayer");

    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    setAspectRatioMode(Plasma::IgnoreAspectRatio);
    setHasConfigurationInterface(true);
    setAcceptDrops(true);

    MetaDataManager::createInstance(this);

    if (args.count())
    {
        KUrl::List tracks;

        for (int i = 0; i < args.count(); ++i)
        {
            tracks.append(KUrl(args.value(i).toString()));
        }

        m_playlistManager->createPlaylist(i18n("Default"), tracks);
    }

    Plasma::ToolTipManager::self()->registerWidget(this);

    resize(250, 50);
}

void Applet::init()
{
    if (m_videoWidget)
    {
        return;
    }

    QGraphicsView *parentView = NULL;
    QGraphicsView *possibleParentView = NULL;
    const QList<QGraphicsView*> views = (scene()?scene()->views():QList<QGraphicsView*>());

    for (int i = 0; i < views.count(); ++i)
    {
        if (views.at(i)->sceneRect().intersects(sceneBoundingRect()) || views.at(i)->sceneRect().contains(scenePos()))
        {
            if (views.at(i)->isActiveWindow())
            {
                parentView = views.at(i);

                break;
            }
            else
            {
                possibleParentView = views.at(i);
            }
        }
    }

    if (!parentView)
    {
        parentView = possibleParentView;
    }

    m_videoWidget = new VideoWidget(parentView, this);

    m_player->registerAppletVideoWidget(m_videoWidget);

    m_controlsWidget = new QGraphicsWidget(this);

    QGraphicsLinearLayout *mainLayout = new QGraphicsLinearLayout(Qt::Vertical, this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->insertItem(0, m_videoWidget);
    mainLayout->insertItem(1, m_controlsWidget);

    SeekSlider *seekSlider = new SeekSlider;
    seekSlider->setPlayer(m_player);

    Plasma::Slider *positionSlider = new Plasma::Slider(m_controlsWidget);
    positionSlider->setWidget(seekSlider);

    Plasma::ToolButton *openButton = new Plasma::ToolButton(m_controlsWidget);
    openButton->setAction(m_player->action(OpenFileAction));

    Plasma::ToolButton *playPauseButton = new Plasma::ToolButton(m_controlsWidget);
    playPauseButton->setAction(m_player->action(PlayPauseAction));

    Plasma::ToolButton *stopButton = new Plasma::ToolButton(m_controlsWidget);
    stopButton->setAction(m_player->action(StopAction));

    Plasma::ToolButton *playPreviousButton = new Plasma::ToolButton(m_controlsWidget);
    playPreviousButton->setAction(m_player->action(PlayPreviousAction));

    Plasma::ToolButton *playNextButton = new Plasma::ToolButton(m_controlsWidget);
    playNextButton->setAction(m_player->action(PlayNextAction));

    Plasma::ToolButton *volumeButton = new Plasma::ToolButton(m_controlsWidget);
    volumeButton->setAction(m_player->action(VolumeToggleAction));

    Plasma::ToolButton *playlistButton = new Plasma::ToolButton(m_controlsWidget);
    playlistButton->setAction(m_player->action(PlaylistToggleAction));

    Plasma::ToolButton *fullScreenButton = new Plasma::ToolButton(m_controlsWidget);
    fullScreenButton->setAction(m_player->action(FullScreenAction));

    m_controls["open"] = openButton;
    m_controls["open"]->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
    m_controls["playPause"] = playPauseButton;
    m_controls["playPause"]->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
    m_controls["stop"] = stopButton;
    m_controls["stop"]->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
    m_controls["playPrevious"] = playPreviousButton;
    m_controls["playPrevious"]->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
    m_controls["playNext"] = playNextButton;
    m_controls["playNext"]->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
    m_controls["position"] = positionSlider;
    m_controls["position"]->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
    m_controls["volume"] = volumeButton;
    m_controls["volume"]->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
    m_controls["playlist"] = playlistButton;
    m_controls["playlist"]->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
    m_controls["fullScreen"] = fullScreenButton;
    m_controls["fullScreen"]->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));

    QGraphicsLinearLayout *controlsLayout = new QGraphicsLinearLayout(Qt::Horizontal, m_controlsWidget);
    controlsLayout->setSpacing(0);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->addItem(m_controls["open"]);
    controlsLayout->addItem(m_controls["playPause"]);
    controlsLayout->addItem(m_controls["stop"]);
    controlsLayout->addItem(m_controls["playPrevious"]);
    controlsLayout->addItem(m_controls["playNext"]);
    controlsLayout->addItem(m_controls["position"]);
    controlsLayout->addItem(m_controls["volume"]);
    controlsLayout->addItem(m_controls["playlist"]);
    controlsLayout->addItem(m_controls["fullScreen"]);

    updateControls();

    QTimer::singleShot(100, this, SLOT(configChanged()));

    connect(this, SIGNAL(activate()), this, SLOT(togglePlaylistDialog()));
    connect(m_player, SIGNAL(currentTrackChanged()), this, SLOT(showToolTip()));
    connect(m_player, SIGNAL(stateChanged(PlayerState)), this, SLOT(stateChanged(PlayerState)));
    connect(m_player, SIGNAL(metaDataChanged()), this, SLOT(metaDataChanged()));
    connect(m_player, SIGNAL(requestMenu(QPoint)), this, SLOT(showMenu(QPoint)));
    connect(m_player, SIGNAL(fullScreenChanged(bool)), this, SLOT(hideToolTip()));
    connect(m_player->action(OpenFileAction), SIGNAL(triggered()), this, SLOT(openFiles()));
    connect(m_player->action(OpenUrlAction), SIGNAL(triggered()), this, SLOT(openUrl()));
    connect(m_player->action(SeekToAction), SIGNAL(triggered()), this, SLOT(toggleJumpToPosition()));
    connect(m_player->action(FullScreenAction), SIGNAL(triggered()), this, SLOT(toggleFullScreen()));
    connect(m_player->action(VolumeToggleAction), SIGNAL(triggered()), this, SLOT(toggleVolumeDialog()));
    connect(m_player->action(PlaylistToggleAction), SIGNAL(triggered()), this, SLOT(togglePlaylistDialog()));
    connect(m_playlistManager, SIGNAL(requestMenu(QPoint)), this, SLOT(showMenu(QPoint)));
}

void Applet::createConfigurationInterface(KConfigDialog *parent)
{
    Configuration *configuration = new Configuration(this, parent);

    connect(configuration, SIGNAL(accepted()), this, SIGNAL(configNeedsSaving()));
    connect(configuration, SIGNAL(accepted()), this, SLOT(configChanged()));
}

void Applet::configChanged()
{
    KConfigGroup configuration = config();

    m_player->setAspectRatio(static_cast<AspectRatio>(config().readEntry("aspectRatio", static_cast<int>(AutomaticRatio))));
    m_player->setAudioMuted(config().readEntry("mute", false));
    m_player->setInhibitNotifications(config().readEntry("inhibitNotifications", false));
    m_player->setVolume(config().readEntry("volume", 50));
    m_player->setBrightness(config().readEntry("brightness", 50));
    m_player->setContrast(config().readEntry("contrast", 50));
    m_player->setHue(config().readEntry("hue", 50));
    m_player->setSaturation(config().readEntry("saturation", 50));

    if (!configuration.readEntry("enableDBus", false) && m_dBusInterface)
    {
        m_dBusInterface->deleteLater();

        m_dBusInterface = NULL;
    }
    else if (configuration.readEntry("enableDBus", false) && !m_dBusInterface)
    {
        m_dBusInterface = new DBusInterface(this);
    }

    if (!m_initialized)
    {
        m_initialized = true;

        KConfigGroup playlistsConfiguration = config().group("Playlists");
        KConfigGroup metaDataConfiguration = config().group("MetaData");
        QStringList playlists = playlistsConfiguration.groupList();
        QStringList tracks = metaDataConfiguration.groupList();
        int currentPlaylist = 0;

        for (int i = 0; i < tracks.count(); ++i)
        {
            KConfigGroup trackConfiguration = metaDataConfiguration.group(tracks.at(i));
            Track track;
            track.keys[ArtistKey] = trackConfiguration.readEntry("artist", QString());
            track.keys[TitleKey] = trackConfiguration.readEntry("title", QString());
            track.keys[AlbumKey] = trackConfiguration.readEntry("album", QString());
            track.keys[TrackNumberKey] = trackConfiguration.readEntry("trackNumber", QString());
            track.keys[GenreKey] = trackConfiguration.readEntry("genre", QString());
            track.keys[DescriptionKey] = trackConfiguration.readEntry("description", QString());
            track.keys[DateKey] = trackConfiguration.readEntry("date", QString());
            track.duration = trackConfiguration.readEntry("duration", -1);

            MetaDataManager::setMetaData(KUrl(trackConfiguration.readEntry("url", QString())), track);
        }

        QMap<int, int> playlistsOrder;

        for (int i = 0; i < playlists.count(); ++i)
        {
            KConfigGroup playlistConfiguration = playlistsConfiguration.group(playlists.at(i));
            int playlistId = m_playlistManager->createPlaylist(playlistConfiguration.readEntry("title", i18n("Default")), KUrl::List(playlistConfiguration.readEntry("tracks", QStringList())), LocalSource, playlistConfiguration.readEntry("id", i));
            PlaylistModel *playlist = m_playlistManager->playlist(playlistId);

            playlistsOrder[playlistId] = playlistConfiguration.readEntry("order", i);

            playlist->setCreationDate(playlistConfiguration.readEntry("creationDate", QDateTime()));
            playlist->setModificationDate(playlistConfiguration.readEntry("modificationDate", QDateTime()));
            playlist->setLastPlayedDate(playlistConfiguration.readEntry("lastPlayedDate", QDateTime()));
            playlist->setCurrentTrack(playlistConfiguration.readEntry("currentTrack", 0));
            playlist->setPlaybackMode(static_cast<PlaybackMode>(playlistConfiguration.readEntry("playbackMode", static_cast<int>(LoopPlaylistMode))));

            if (playlistConfiguration.readEntry("isCurrent", false))
            {
                currentPlaylist = playlistId;
            }
        }

        if (m_playlistManager->playlists().count())
        {
            QMultiMap<int, int> orderedPlaylists;
            QMap<int, int>::iterator iterator;

            for (iterator = playlistsOrder.begin(); iterator != playlistsOrder.end(); ++iterator)
            {
                orderedPlaylists.insert(iterator.value(), iterator.key());
            }

            m_playlistManager->setPlaylistsOrder(orderedPlaylists.values());
        }
        else
        {
            m_playlistManager->createPlaylist(i18n("Default"), KUrl::List());
        }

        m_playlistManager->setCurrentPlaylist(currentPlaylist);

        if (config().readEntry("playOnStartup", false) && m_player->playlist() && m_player->playlist()->trackCount())
        {
            m_player->play();
        }

        connect(m_player, SIGNAL(modified()), this, SLOT(configSave()));
        connect(m_playlistManager, SIGNAL(modified()), this, SLOT(configSave()));
    }

    updateControls();
}

void Applet::configSave()
{
    KConfigGroup configuration = config();
    configuration.writeEntry("aspectRatio", static_cast<int>(m_player->aspectRatio()));
    configuration.writeEntry("mute", m_player->isAudioMuted());
    configuration.writeEntry("volume", m_player->volume());
    configuration.writeEntry("brightness", m_player->brightness());
    configuration.writeEntry("contrast", m_player->contrast());
    configuration.writeEntry("hue", m_player->hue());
    configuration.writeEntry("saturation", m_player->saturation());

    if (m_playlistManager->isDialogVisible())
    {
        configuration.writeEntry("playlistSize", m_playlistManager->dialogSize());
        configuration.writeEntry("columnsOrder", m_playlistManager->columnsOrder());
        configuration.writeEntry("columnsVisibility", m_playlistManager->columnsVisibility());
        configuration.writeEntry("playlistLocked", m_playlistManager->isSplitterLocked());
        configuration.writeEntry("playlistSplitter", m_playlistManager->splitterState());
        configuration.writeEntry("playlistViewHeader", m_playlistManager->headerState());
    }

    configuration.deleteGroup("Playlists");
    configuration.deleteGroup("MetaData");

    KConfigGroup playlistsConfiguration = configuration.group("Playlists");
    KConfigGroup metaDataConfiguration = configuration.group("MetaData");
    const QList<int> playlists = m_playlistManager->playlists();
    QSet<KUrl> playlistTracks;
    int index = 0;

    for (int i = 0; i < playlists.count(); ++i)
    {
        PlaylistModel *playlist = m_playlistManager->playlist(playlists[i]);

        if (!playlist || playlist->isReadOnly())
        {
            continue;
        }

        KConfigGroup playlistConfiguration = playlistsConfiguration.group(QString::number(playlist->id()));
        playlistConfiguration.writeEntry("order", i);
        playlistConfiguration.writeEntry("id", playlist->id());
        playlistConfiguration.writeEntry("tracks", playlist->tracks().toStringList());
        playlistConfiguration.writeEntry("title", playlist->title());
        playlistConfiguration.writeEntry("creationDate", playlist->creationDate());
        playlistConfiguration.writeEntry("modificationDate", playlist->modificationDate());
        playlistConfiguration.writeEntry("lastPlayedDate", playlist->lastPlayedDate());
        playlistConfiguration.writeEntry("playbackMode", static_cast<int>(playlist->playbackMode()));
        playlistConfiguration.writeEntry("currentTrack", playlist->currentTrack());
        playlistConfiguration.writeEntry("isCurrent", playlist->isCurrent());

        playlistTracks = playlistTracks.unite(playlist->tracks().toSet());

        ++index;
    }

    const KUrl::List tracks = MetaDataManager::tracks();

    for (int i = 0; i < tracks.count(); ++i)
    {
        if (!playlistTracks.contains(tracks.at(i)))
        {
            continue;
        }

        KConfigGroup trackConfiguration = metaDataConfiguration.group(QString::number(i));
        trackConfiguration.writeEntry("url", tracks.at(i));
        trackConfiguration.writeEntry("artist", MetaDataManager::metaData(tracks.at(i), ArtistKey, false));
        trackConfiguration.writeEntry("title", MetaDataManager::metaData(tracks.at(i), TitleKey, false));
        trackConfiguration.writeEntry("album", MetaDataManager::metaData(tracks.at(i), AlbumKey, false));
        trackConfiguration.writeEntry("trackNumber", MetaDataManager::metaData(tracks.at(i), TrackNumberKey, false));
        trackConfiguration.writeEntry("genre", MetaDataManager::metaData(tracks.at(i), GenreKey, false));
        trackConfiguration.writeEntry("description", MetaDataManager::metaData(tracks.at(i), DescriptionKey, false));
        trackConfiguration.writeEntry("date", MetaDataManager::metaData(tracks.at(i), DateKey, false));
        trackConfiguration.writeEntry("duration", MetaDataManager::duration(tracks.at(i)));
    }

    Q_EMIT configNeedsSaving();
}

void Applet::constraintsEvent(Plasma::Constraints constraints)
{
    if ((constraints & Plasma::LocationConstraint || constraints & Plasma::SizeConstraint) && m_playlistManager->isDialogVisible())
    {
        m_playlistManager->showDialog(popupPosition(m_playlistManager->dialogSize()));
    }
}

void Applet::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    updateVideo();

    Plasma::Applet::resizeEvent(event);
}

void Applet::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    if (!event->mimeData()->hasUrls())
    {
        event->ignore();

        return;
    }

    if (!m_playlistManager->isDialogVisible())
    {
        m_togglePlaylist = startTimer(500);
    }
}

void Applet::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
{
    Q_UNUSED(event)

    killTimer(m_togglePlaylist);
}

void Applet::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    killTimer(m_togglePlaylist);

    m_playlistManager->addTracks(KUrl::List::fromMimeData(event->mimeData()), -1, PlayReaction);
}

void Applet::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        togglePlaylistDialog();
    }

    event->ignore();
}

void Applet::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
        case Qt::Key_PageDown:
        case Qt::Key_MediaPrevious:
            m_player->playPrevious();

            break;
        case Qt::Key_PageUp:
        case Qt::Key_MediaNext:
            m_player->playNext();

            break;
        case Qt::Key_Left:
            m_player->seekBackward();

            break;
        case Qt::Key_Right:
            m_player->seekForward();

            break;
        case Qt::Key_Plus:
            m_player->increaseVolume();

            break;
        case Qt::Key_Minus:
            m_player->decreaseVolume();

            break;
        case Qt::Key_Space:
        case Qt::Key_MediaTogglePlayPause:
            m_player->playPause();

            break;
        case Qt::Key_MediaPlay:
            m_player->play();

            break;
        case Qt::Key_MediaPause:
            m_player->pause();

            break;
        case Qt::Key_S:
        case Qt::Key_MediaStop:
            m_player->stop();

            break;
        case Qt::Key_Escape:
            if (m_player->isFullScreen())
            {
                m_player->setFullScreen(false);
            }
            else if (m_playlistManager->isDialogVisible())
            {
                m_playlistManager->closeDialog();
            }

            break;
        case Qt::Key_F:
            m_player->setFullScreen(!m_player->isFullScreen());

            break;
        case Qt::Key_G:
            toggleJumpToPosition();

            break;
        case Qt::Key_M:
            m_player->setAudioMuted(!m_player->isAudioMuted());

            break;
        case Qt::Key_O:
            openFiles();

            break;
        case Qt::Key_P:
            togglePlaylistDialog();

            break;
        case Qt::Key_U:
            openUrl();

            break;
        case Qt::Key_V:
            toggleVolumeDialog();

            break;
        default:
            event->ignore();

            break;
    }
}

void Applet::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_togglePlaylist)
    {
        togglePlaylistDialog();

        killTimer(event->timerId());
    }
    else if (event->timerId() == m_hideToolTip)
    {
        if (!isUnderMouse())
        {
            Plasma::ToolTipManager::self()->hide(this);

            m_hideToolTip = 0;

            killTimer(event->timerId());
        }
    }
    else if (event->timerId() == m_updateToolTip)
    {
        updateToolTip();
    }
    else
    {
        killTimer(event->timerId());
    }
}

void Applet::stateChanged(PlayerState state)
{
    if (state == PlayingState && m_hideToolTip == 0)
    {
        QTimer::singleShot(500, this, SLOT(showToolTip()));
    }
    else if (state == StoppedState)
    {
        Plasma::ToolTipManager::self()->clearContent(this);
    }
}

void Applet::metaDataChanged()
{
    if (m_player->state() != StoppedState && m_player->position() < 150 && m_hideToolTip == 0)
    {
        updateToolTip();

        QTimer::singleShot(500, this, SLOT(showToolTip()));
    }
}

void Applet::openUrl()
{
    const QString url = KInputDialog::getText(i18n("Open URL"), i18n("Enter a URL:"));

    if (!url.isEmpty())
    {
        m_playlistManager->addTracks(KUrl::List(url), -1, (m_playlistManager->isDialogVisible()?NoReaction:PlayReaction));
    }
}

void Applet::openFiles()
{
    KFileDialog dialog(KUrl(config().readEntry("directory", "~")), QString(), NULL);
    dialog.setFilter(m_player->supportedMimeTypes().join(QChar(' ')));
    dialog.setWindowModality(Qt::NonModal);
    dialog.setMode(KFile::Files | KFile::Directory);
    dialog.setOperationMode(KFileDialog::Opening);
    dialog.exec();

    KUrl::List urls = dialog.selectedUrls();

    if (urls.isEmpty())
    {
        return;
    }

    config().writeEntry("directory", QFileInfo(urls.at(0).toLocalFile()).absolutePath());

    m_playlistManager->addTracks(urls, -1, (m_playlistManager->isDialogVisible()?NoReaction:PlayReaction));

    Q_EMIT configNeedsSaving();
}

void Applet::jumpToPosition()
{
    if (m_player->isSeekable())
    {
        m_player->setPosition(-m_jumpToPositionUi.position->time().msecsTo(QTime()));
    }
}

void Applet::toggleJumpToPosition()
{
    if (!m_jumpToPositionDialog)
    {
        m_jumpToPositionDialog = new KDialog;
        m_jumpToPositionDialog->setCaption(i18n("Jump to Position"));
        m_jumpToPositionDialog->setButtons(KDialog::Ok | KDialog::Cancel);
        m_jumpToPositionDialog->setWindowModality(Qt::NonModal);

        QWidget *jumpToPositionWidget = new QWidget;

        m_jumpToPositionUi.setupUi(jumpToPositionWidget);

        m_jumpToPositionDialog->setMainWidget(jumpToPositionWidget);

        connect(this, SIGNAL(destroyed()), m_jumpToPositionDialog, SLOT(deleteLater()));
        connect(m_jumpToPositionDialog, SIGNAL(okClicked()), this, SLOT(jumpToPosition()));
    }

    if ((m_player->state() == PlayingState || m_player->state() == PausedState) && !m_jumpToPositionDialog->isVisible())
    {
        QTime tmpTime(0, 0);

        m_jumpToPositionUi.position->setMaximumTime(tmpTime.addMSecs(m_player->duration()));
        m_jumpToPositionUi.position->setTime(tmpTime.addMSecs(m_player->position()));

        m_jumpToPositionDialog->show();
    }
    else
    {
        m_jumpToPositionDialog->close();
    }
}

void Applet::toggleVolumeDialog()
{
    if (!m_volumeDialog)
    {
        m_volumeDialog = new Plasma::Dialog;
        m_volumeDialog->setWindowFlags(Qt::Popup);

        m_volumeUi.setupUi(m_volumeDialog);

        m_volumeUi.volumeSlider->setOrientation(Qt::Vertical);
        m_volumeUi.volumeSlider->setPlayer(m_player);
        m_volumeUi.muteButton->setDefaultAction(m_player->action(MuteAction));

        m_volumeDialog->adjustSize();

        connect(this, SIGNAL(destroyed()), m_volumeDialog, SLOT(deleteLater()));
    }

    if (m_volumeDialog->isVisible())
    {
        m_volumeDialog->close();
    }
    else
    {
        m_volumeDialog->move(containment()->corona()->popupPosition(m_controls["volume"], m_volumeDialog->size(), Qt::AlignCenter));
        m_volumeDialog->show();
    }
}

void Applet::toggleFullScreen()
{
    m_player->setFullScreen(!m_player->isFullScreen());
}

void Applet::togglePlaylistDialog()
{
    if (m_playlistManager->isDialogVisible())
    {
        m_playlistManager->closeDialog();
    }
    else
    {
        m_playlistManager->setDialogSize(config().readEntry("playlistSize", m_playlistManager->dialogSize()));
        m_playlistManager->setColumnsOrder(config().readEntry("columnsOrder", m_playlistManager->columnsOrder()));
        m_playlistManager->setColumnsVisibility(config().readEntry("columnsVisibility", m_playlistManager->columnsVisibility()));
        m_playlistManager->setSplitterLocked(config().readEntry("playlistLocked", true));
        m_playlistManager->setSplitterState(config().readEntry("playlistSplitter", QByteArray()));
        m_playlistManager->setHeaderState(config().readEntry("headerState", QByteArray()));
        m_playlistManager->showDialog(containment()->corona()->popupPosition(this, config().readEntry("playlistSize", m_playlistManager->dialogSize()), Qt::AlignCenter));
    }
}

void Applet::toolTipAboutToShow()
{
    m_updateToolTip = startTimer(1000);

    updateToolTip();
}

void Applet::toolTipHidden()
{
    Plasma::ToolTipManager::self()->clearContent(this);

    killTimer(m_updateToolTip);

    m_updateToolTip = 0;
}

void Applet::showToolTip()
{
    const qint64 time = (config().readEntry("showToolTipOnTrackChange", 3) * 1000);

    if (time > 0)
    {
        killTimer(m_hideToolTip);

        if (!m_player->isFullScreen() && !(KWindowSystem::windowInfo(KWindowSystem::activeWindow(), NET::WMState).state() & NET::FullScreen))
        {
            Plasma::ToolTipManager::self()->show(this);

            m_hideToolTip = startTimer(time);
        }
    }
}

void Applet::hideToolTip()
{
    Plasma::ToolTipManager::self()->hide(this);
}

void Applet::updateToolTip()
{
    if (!m_updateToolTip && !Plasma::ToolTipManager::self()->isVisible(this))
    {
        return;
    }

    Plasma::ToolTipContent data;

    if (m_player->state() != StoppedState)
    {
        const QString artist = m_player->metaData(ArtistKey, false);
        const QString title = m_player->metaData(TitleKey, false);
        QString text;

        if (artist.isEmpty() || title.isEmpty())
        {
            text = (artist.isEmpty()?title:artist);
        }
        else
        {
            text = QString("%1 - %2").arg(artist).arg(title);
        }

        if (text.isEmpty())
        {
            text = m_player->metaData(TitleKey);
        }

        data.setMainText(text);
        data.setSubText((m_player->duration() > 0)?i18n("Position: %1 / %2", MetaDataManager::timeToString(m_player->position()), MetaDataManager::timeToString(m_player->duration())):"");
        data.setImage(MetaDataManager::icon(m_player->url()).pixmap(IconSize(KIconLoader::Desktop)));
        data.setAutohide(true);
    }

    Plasma::ToolTipManager::self()->setContent(this, data);
}

void Applet::updateControls()
{
    if (!m_controlsWidget)
    {
        return;
    }

    QStringList controls;
    controls << "open" << "playPause" << "stop" << "position" << "volume" << "playlist";
    controls = config().readEntry("controls", controls);

    QMap<QString, QGraphicsProxyWidget*>::iterator iterator;
    bool visible = false;

    for (iterator = m_controls.begin(); iterator != m_controls.end(); ++iterator)
    {
        iterator.value()->setVisible(controls.contains(iterator.key()));
        iterator.value()->setMaximumWidth(controls.contains(iterator.key())?-1:0);

        if (controls.contains(iterator.key()))
        {
            visible = true;
        }
    }

    m_controlsWidget->setVisible(visible);
    m_controlsWidget->setMaximumHeight(visible?-1:0);

    updateVideo();
}

void Applet::updateVideo()
{
    if (!m_controlsWidget)
    {
        return;
    }

    const bool showVideo = (!m_controlsWidget->isVisible() || size().height() > 70);

    if (m_videoWidget)
    {
        QGraphicsLinearLayout *mainLayout = static_cast<QGraphicsLinearLayout*>(layout());

        if (showVideo)
        {
            mainLayout->insertItem(0, m_videoWidget);

            m_videoWidget->show();
        }
        else
        {
            m_videoWidget->hide();

            mainLayout->removeItem(m_videoWidget);
        }
    }

    m_player->setVideoMode(showVideo);
}

void Applet::showMenu(const QPoint &position)
{
    KMenu menu;
    menu.addActions(contextualActions());

    if (m_playlistManager->isDialogVisible() && !m_player->isFullScreen())
    {
        menu.addSeparator();

        QAction *lockAction = menu.addAction(KIcon("object-locked"), i18n("Lock widgets"));
        lockAction->setCheckable(true);
        lockAction->setChecked(m_playlistManager->isSplitterLocked());

        connect(lockAction, SIGNAL(toggled(bool)), m_playlistManager, SLOT(setSplitterLocked(bool)));
    }

    menu.addSeparator();
    menu.addAction(KIcon("configure"), i18n("Settings"), this, SLOT(showConfigurationInterface()));
    menu.exec(position);
}

QList<QAction*> Applet::contextualActions()
{
    if (m_actions.isEmpty())
    {
        QAction *separator1 = new QAction(this);
        separator1->setSeparator(true);

        QAction *separator2 = new QAction(this);
        separator2->setSeparator(true);

        QAction *separator3 = new QAction(this);
        separator3->setSeparator(true);

        QAction *separator4 = new QAction(this);
        separator4->setSeparator(true);

        QAction *separator5 = new QAction(this);
        separator5->setSeparator(true);

        QAction *separator6 = new QAction(this);
        separator6->setSeparator(true);

        m_actions.append(m_player->action(OpenMenuAction));
        m_actions.append(separator1);
        m_actions.append(m_player->action(PlayPauseAction));
        m_actions.append(m_player->action(StopAction));
        m_actions.append(separator2);
        m_actions.append(m_player->action(NavigationMenuAction));
        m_actions.append(separator3);
        m_actions.append(m_player->action(AudioMenuAction));
        m_actions.append(separator4);
        m_actions.append(m_player->action(VideoMenuAction));
        m_actions.append(separator5);
        m_actions.append(m_player->action(PlaylistToggleAction));
        m_actions.append(separator6);
    }

    return m_actions;
}

Player* Applet::player()
{
    return m_player;
}

PlaylistManager* Applet::playlistManager()
{
    return m_playlistManager;
}

bool Applet::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        keyPressEvent(static_cast<QKeyEvent*>(event));

        return true;
    }

    return QObject::eventFilter(object, event);
}

}
