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

#include "Applet.h"
#include "Player.h"
#include "VideoWidget.h"
#include "MetaDataManager.h"
#include "PlaylistManager.h"
#include "PlaylistModel.h"
#include "PlaylistReader.h"
#include "SeekSlider.h"
#include "RootDBusHandler.h"
#include "PlayerDBusHandler.h"
#include "TrackListDBusHandler.h"

#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QTimerEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QGraphicsLinearLayout>
#include <QtGui/QGraphicsSceneResizeEvent>
#include <QtGui/QGraphicsSceneDragDropEvent>
#include <QtDBus/QDBusConnection>

#include <KIcon>
#include <KMenu>
#include <KLocale>
#include <KFileDialog>
#include <KInputDialog>
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
    m_videoWidget(new VideoWidget(this)),
    m_volumeDialog(NULL),
    m_playerDBUSHandler(NULL),
    m_trackListDBusHandler(NULL),
    m_rootDBUSHandler(NULL),
    m_controlsWidget(new QGraphicsWidget(this)),
    m_fullScreenWidget(NULL),
    m_jumpToPositionDialog(NULL),
    m_hideFullScreenControls(0),
    m_togglePlaylist(0),
    m_hideToolTip(0),
    m_updateToolTip(0)
{
    KGlobal::locale()->insertCatalog("miniplayer");

    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    setAspectRatioMode(Plasma::IgnoreAspectRatio);
    setHasConfigurationInterface(true);
    setAcceptDrops(true);

    MetaDataManager::createInstance(this);

    m_player->registerAppletVideoWidget(m_videoWidget);
    m_player->setVideoMode(false);

    QGraphicsLinearLayout *mainLayout = new QGraphicsLinearLayout(Qt::Vertical, this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->insertItem(0, m_videoWidget);
    mainLayout->insertItem(1, m_controlsWidget);

    setLayout(mainLayout);

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
    controlsLayout->addItem(m_controls["position"]);
    controlsLayout->addItem(m_controls["volume"]);
    controlsLayout->addItem(m_controls["playlist"]);
    controlsLayout->addItem(m_controls["fullScreen"]);

    m_controlsWidget->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
    m_controlsWidget->setLayout(controlsLayout);

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

    setMinimumWidth(150);
    resize(250, 50);

    connect(this, SIGNAL(activate()), this, SLOT(togglePlaylistDialog()));
    connect(m_player, SIGNAL(trackChanged()), this, SLOT(showToolTip()));
    connect(m_player, SIGNAL(stateChanged(PlayerState)), this, SLOT(stateChanged(PlayerState)));
    connect(m_player, SIGNAL(videoAvailableChanged(bool)), this, SLOT(videoAvailableChanged(bool)));
    connect(m_player, SIGNAL(metaDataChanged()), this, SLOT(metaDataChanged()));
    connect(m_player, SIGNAL(durationChanged(qint64)), this, SLOT(metaDataChanged()));
    connect(m_player, SIGNAL(requestMenu(QPoint)), this, SLOT(showMenu(QPoint)));
    connect(m_player->action(OpenFileAction), SIGNAL(triggered()), this, SLOT(openFiles()));
    connect(m_player->action(OpenUrlAction), SIGNAL(triggered()), this, SLOT(openUrl()));
    connect(m_player->action(SeekToAction), SIGNAL(triggered()), this, SLOT(toggleJumpToPosition()));
    connect(m_player->action(FullScreenAction), SIGNAL(triggered()), this, SLOT(toggleFullScreen()));
    connect(m_player->action(VolumeToggleAction), SIGNAL(triggered()), this, SLOT(toggleVolumeDialog()));
    connect(m_player->action(PlaylistToggleAction), SIGNAL(triggered()), this, SLOT(togglePlaylistDialog()));
    connect(m_playlistManager, SIGNAL(requestMenu(QPoint)), this, SLOT(showMenu(QPoint)));
}

Applet::~Applet()
{
    if (m_playerDBUSHandler)
    {
        delete m_playerDBUSHandler;
        delete m_trackListDBusHandler;
        delete m_rootDBUSHandler;
    }
}

void Applet::init()
{
    KConfigGroup playlistsConfiguration = config().group("Playlists");
    KConfigGroup metaDataConfiguration = config().group("MetaData");
    QStringList playlists = playlistsConfiguration.groupList();
    QStringList tracks = metaDataConfiguration.groupList();
    int currentPlaylist = 0;

    for (int i = 0; i < tracks.count(); ++i)
    {
        KConfigGroup trackConfiguration = metaDataConfiguration.group(tracks.at(i));
        Track track;
        track.title = trackConfiguration.readEntry("title", QString());
        track.artist = trackConfiguration.readEntry("artist", QString());
        track.duration = trackConfiguration.readEntry("duration", -1);

        MetaDataManager::setMetaData(KUrl(trackConfiguration.readEntry("url", QString())), track);
    }

    for (int i = 0; i < playlists.count(); ++i)
    {
        KConfigGroup playlistConfiguration = playlistsConfiguration.group(playlists.at(i));
        int playlist = m_playlistManager->createPlaylist(playlistConfiguration.readEntry("title", i18n("Default")), KUrl::List(playlistConfiguration.readEntry("tracks", QStringList())));

        m_playlistManager->playlists().at(playlist)->setCurrentTrack(playlistConfiguration.readEntry("currentTrack", 0));
        m_playlistManager->playlists().at(playlist)->setPlaybackMode(static_cast<PlaybackMode>(playlistConfiguration.readEntry("playbackMode", static_cast<int>(LoopPlaylistMode))));

        if (playlistConfiguration.readEntry("isCurrent", false))
        {
            currentPlaylist = playlist;
        }
    }

    if (!m_playlistManager->playlists().count())
    {
        m_playlistManager->createPlaylist(i18n("Default"), KUrl::List());
    }

    m_playlistManager->setCurrentPlaylist(currentPlaylist);

    m_player->setAspectRatio(static_cast<AspectRatio>(config().readEntry("apectRatio", static_cast<int>(AutomaticRatio))));
    m_player->setAudioMuted(config().readEntry("mute", false));
    m_player->setVolume(config().readEntry("volume", 50));
    m_player->setBrightness(config().readEntry("brightness", 50));
    m_player->setContrast(config().readEntry("contrast", 50));
    m_player->setHue(config().readEntry("hue", 50));
    m_player->setSaturation(config().readEntry("saturation", 50));

    if (config().readEntry("playOnStartup", false) && m_player->playlist() && m_player->playlist()->trackCount())
    {
        m_player->play();
    }

    QTimer::singleShot(100, this, SLOT(configReset()));

    connect(m_player, SIGNAL(needsSaving()), this, SLOT(configSave()));
    connect(m_playlistManager, SIGNAL(needsSaving()), this, SLOT(configSave()));
}

void Applet::createConfigurationInterface(KConfigDialog *parent)
{
    KConfigGroup configuration = config();
    QWidget *generalWidget = new QWidget;
    QWidget *controlsWidget = new QWidget;
    QStringList controls;
    controls << "open" << "playPause" << "stop" << "position" << "volume" << "playlist";
    controls = configuration.readEntry("controls", controls);

    m_generalUi.setupUi(generalWidget);
    m_controlsUi.setupUi(controlsWidget);

    m_generalUi.playOnStartup->setChecked(configuration.readEntry("playOnStartup", false));
    m_generalUi.enableDBUS->setChecked(configuration.readEntry("enableDBus", false));
    m_generalUi.showToolTipOnTrackChange->setValue(configuration.readEntry("showToolTipOnTrackChange", 3));

    m_controlsUi.openCheckBox->setChecked(controls.contains("open"));
    m_controlsUi.playPauseCheckBox->setChecked(controls.contains("playPause"));
    m_controlsUi.stopCheckBox->setChecked(controls.contains("stop"));
    m_controlsUi.positionCheckBox->setChecked(controls.contains("position"));
    m_controlsUi.volumeCheckBox->setChecked(controls.contains("volume"));
    m_controlsUi.playlistCheckBox->setChecked(controls.contains("playlist"));
    m_controlsUi.fullScreenCheckBox->setChecked(controls.contains("fullScreen"));

    parent->addPage(generalWidget, i18n("General"), "go-home");
    parent->addPage(controlsWidget, i18n("Visible Controls"), "media-playback-start");

    connect(parent, SIGNAL(applyClicked()), this, SLOT(configAccepted()));
    connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));
}

void Applet::configAccepted()
{
    KConfigGroup configuration = config();
    QStringList controls;

    if (m_controlsUi.openCheckBox->isChecked())
    {
        controls.append("open");
    }

    if (m_controlsUi.playPauseCheckBox->isChecked())
    {
        controls.append("playPause");
    }

    if (m_controlsUi.stopCheckBox->isChecked())
    {
        controls.append("stop");
    }

    if (m_controlsUi.positionCheckBox->isChecked())
    {
        controls.append("position");
    }

    if (m_controlsUi.volumeCheckBox->isChecked())
    {
        controls.append("volume");
    }

    if (m_controlsUi.playlistCheckBox->isChecked())
    {
        controls.append("playlist");
    }

    if (m_controlsUi.fullScreenCheckBox->isChecked())
    {
        controls.append("fullScreen");
    }

    configuration.writeEntry("controls", controls);
    configuration.writeEntry("playOnStartup", m_generalUi.playOnStartup->isChecked());
    configuration.writeEntry("enableDBus", m_generalUi.enableDBUS->isChecked());
    configuration.writeEntry("showToolTipOnTrackChange", m_generalUi.showToolTipOnTrackChange->value());

    emit configNeedsSaving();

    configReset();
}

void Applet::configSave()
{
    KConfigGroup configuration = config();
    configuration.writeEntry("apectRatio", static_cast<int>(m_player->aspectRatio()));
    configuration.writeEntry("mute", m_player->isAudioMuted());
    configuration.writeEntry("volume", m_player->volume());
    configuration.writeEntry("brightness", m_player->brightness());
    configuration.writeEntry("contrast", m_player->contrast());
    configuration.writeEntry("hue", m_player->hue());
    configuration.writeEntry("saturation", m_player->saturation());

    if (m_playlistManager->isDialogVisible())
    {
        configuration.writeEntry("playlistSize", m_playlistManager->dialogSize());
        configuration.writeEntry("playlistSplitter", m_playlistManager->splitterState());
        configuration.writeEntry("playlistViewHeader", m_playlistManager->headerState());
    }

    configuration.deleteGroup("Playlists");
    configuration.deleteGroup("MetaData");

    KConfigGroup playlistsConfiguration = configuration.group("Playlists");
    KConfigGroup metaDataConfiguration = configuration.group("MetaData");
    QList<PlaylistModel*> playlists = m_playlistManager->playlists();
    QSet<KUrl> playlistTracks;
    int index = 0;

    for (int i = 0; i < playlists.count(); ++i)
    {
        if (playlists[i]->isReadOnly())
        {
            continue;
        }

        KConfigGroup playlistConfiguration = playlistsConfiguration.group(QString::number(index));
        playlistConfiguration.writeEntry("tracks", playlists[i]->tracks().toStringList());
        playlistConfiguration.writeEntry("title", playlists[i]->title());
        playlistConfiguration.writeEntry("playbackMode", static_cast<int>(playlists[i]->playbackMode()));
        playlistConfiguration.writeEntry("currentTrack", playlists[i]->currentTrack());
        playlistConfiguration.writeEntry("isCurrent", (m_playlistManager->currentPlaylist() == i));

        playlistTracks = playlistTracks.unite(playlists.at(i)->tracks().toSet());

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
        trackConfiguration.writeEntry("title", MetaDataManager::title(tracks.at(i)));
        trackConfiguration.writeEntry("artist", MetaDataManager::artist(tracks.at(i)));
        trackConfiguration.writeEntry("duration", MetaDataManager::duration(tracks.at(i)));
    }

    emit configNeedsSaving();
}

void Applet::configReset()
{
    KConfigGroup configuration = config();
    QStringList controls;
    controls << "open" << "playPause" << "stop" << "position" << "volume" << "playlist";
    controls = configuration.readEntry("controls", controls);

    QHash<QString, QGraphicsWidget*>::iterator iterator;
    bool visible = false;

    for (iterator = m_controls.begin(); iterator != m_controls.end(); ++iterator)
    {
        iterator.value()->setMaximumWidth(controls.contains(iterator.key())?-1:0);
        iterator.value()->setVisible(controls.contains(iterator.key()));

        if (controls.contains(iterator.key()))
        {
            visible = true;
        }
    }

    m_controlsWidget->setVisible(visible);
    m_controlsWidget->setMaximumHeight(visible?-1:0);

    if (!configuration.readEntry("enableDBus", false) && m_playerDBUSHandler)
    {
        QDBusConnection dbus = QDBusConnection::sessionBus();
        dbus.unregisterService("org.mpris.PlasmaApplet");
        dbus.unregisterObject("/PlasmaApplet");

        delete m_playerDBUSHandler;
        delete m_trackListDBusHandler;
        delete m_rootDBUSHandler;

        m_playerDBUSHandler = NULL;
        m_trackListDBusHandler = NULL;
        m_rootDBUSHandler = NULL;
    }
    else if (configuration.readEntry("enableDBus", false) && !m_playerDBUSHandler)
    {
        QDBusConnection dbus = QDBusConnection::sessionBus();
        dbus.registerService("org.mpris.PlasmaApplet");
        dbus.registerObject("/PlasmaApplet", m_player);

        m_playerDBUSHandler = new PlayerDBusHandler(m_player);
        m_trackListDBusHandler = new TrackListDBusHandler(m_player);
        m_rootDBUSHandler = new RootDBusHandler(m_player);
    }

    m_player->setVideoMode(!visible|| (size().height() - m_controlsWidget->size().height()) > 50);
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
    m_player->setVideoMode(!m_controlsWidget->isVisible() || (event->newSize().height() - m_controlsWidget->size().height()) > 50);

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
            m_player->playPrevious();
        break;
        case Qt::Key_PageUp:
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
            m_player->playPause();
        break;
        case Qt::Key_Escape:
            if (m_fullScreenWidget && m_fullScreenWidget->isFullScreen())
            {
                toggleFullScreen();
            }
            else if (m_playlistManager->isDialogVisible())
            {
                m_playlistManager->closeDialog();
            }
        break;
        case Qt::Key_F:
            toggleFullScreen();
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
        case Qt::Key_S:
            m_player->stop();
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
    if (event->timerId() == m_hideFullScreenControls && m_fullScreenWidget && !m_fullScreenUi.controlsWidget->underMouse())
    {
        m_fullScreenUi.videoWidget->setCursor(QCursor(Qt::BlankCursor));
        m_fullScreenUi.titleLabel->hide();
        m_fullScreenUi.controlsWidget->hide();

        killTimer(event->timerId());
    }
    else if (event->timerId() == m_togglePlaylist)
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
    if (state == PlayingState)
    {
        QTimer::singleShot(500, this, SLOT(showToolTip()));
    }
    else if (state == StoppedState)
    {
        Plasma::ToolTipManager::self()->clearContent(this);

        videoAvailableChanged(false);

        emit titleChanged(QString());
    }
}

void Applet::videoAvailableChanged(bool videoAvailable)
{
    if (!videoAvailable && m_fullScreenWidget && m_fullScreenWidget->isFullScreen())
    {
        toggleFullScreen();
    }
}

void Applet::metaDataChanged()
{
    if (m_player->state() == StoppedState)
    {
        return;
    }

    if (!MetaDataManager::isAvailable(m_player->url()))
    {
        Track track;
        track.title = m_player->title(false);
        track.artist = m_player->artist();
        track.duration = m_player->duration();

        MetaDataManager::setMetaData(m_player->url(), track);
    }

    if (m_player->position() < 150 && m_hideToolTip == 0)
    {
        updateToolTip();

        QTimer::singleShot(500, this, SLOT(showToolTip()));
    }

    emit titleChanged(m_player->title());
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
    KFileDialog dialog(KUrl(config().readEntry("directory", "~")), "", NULL);
    dialog.setFilter("video/ogg video/x-theora+ogg video/x-ogm+ogg video/x-ms-wmv video/x-msvideo video/x-ms-asf video/x-matroska video/mpeg video/avi video/quicktime video/vnd.rn-realvideo video/x-flic video/mp4 video/x-flv video/3gpp application/ogg audio/x-vorbis+ogg audio/mpeg audio/x-flac audio/x-flac+ogg audio/x-musepack audio/x-scpls audio/x-mpegurl application/xspf+xml audio/x-ms-asx");
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

    emit configNeedsSaving();
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
    if (!m_fullScreenWidget)
    {
        m_fullScreenWidget = new QWidget;
        m_fullScreenWidget->installEventFilter(this);

        m_fullScreenUi.setupUi(m_fullScreenWidget);
        m_fullScreenUi.playPauseButton->setDefaultAction(m_player->action(PlayPauseAction));
        m_fullScreenUi.stopButton->setDefaultAction(m_player->action(StopAction));
        m_fullScreenUi.seekSlider->setPlayer(m_player);
        m_fullScreenUi.muteButton->setDefaultAction(m_player->action(MuteAction));
        m_fullScreenUi.volumeSlider->setPlayer(m_player);
        m_fullScreenUi.fullScreenButton->setDefaultAction(m_player->action(FullScreenAction));
        m_fullScreenUi.titleLabel->setText(m_player->title());

        m_player->registerFullScreenVideoWidget(m_fullScreenUi.videoWidget);

        connect(this, SIGNAL(titleChanged(QString)), m_fullScreenUi.titleLabel, SLOT(setText(QString)));
        connect(this, SIGNAL(destroyed()), m_fullScreenWidget, SLOT(deleteLater()));
    }

    if (m_fullScreenWidget->isFullScreen() || !m_player->isVideoAvailable())
    {
        killTimer(m_hideFullScreenControls);

        m_player->setFullScreen(false);

        m_fullScreenWidget->showNormal();
        m_fullScreenWidget->hide();

        m_player->action(FullScreenAction)->setIcon(KIcon("view-fullscreen"));
        m_player->action(FullScreenAction)->setText(i18n("Full Screen Mode"));

        m_fullScreenUi.videoWidget->setCursor(QCursor(Qt::ArrowCursor));
    }
    else
    {
        Plasma::ToolTipManager::self()->hide(this);

        m_player->setFullScreen(true);

        m_fullScreenWidget->showFullScreen();
        m_fullScreenWidget->setWindowTitle(m_player->title());

        m_fullScreenUi.titleLabel->setText(m_player->title());
        m_fullScreenUi.titleLabel->hide();
        m_fullScreenUi.controlsWidget->hide();

        m_player->action(FullScreenAction)->setIcon(KIcon("view-restore"));
        m_player->action(FullScreenAction)->setText(i18n("Exit Full Screen Mode"));

        m_hideFullScreenControls = startTimer(2000);
    }
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
        m_playlistManager->setSplitterState(config().readEntry("playlistSplitter", QByteArray()));
        m_playlistManager->setHeaderState(config().readEntry("headerState", QByteArray()));
        m_playlistManager->showDialog(containment()->corona()->popupPosition(this, config().readEntry("playlistSize", m_playlistManager->dialogSize()), Qt::AlignCenter));
    }
}

void Applet::showToolTip()
{
    const qint64 time = (config().readEntry("showToolTipOnTrackChange", 3) * 1000);

    if (time > 0)
    {
        killTimer(m_hideToolTip);

        Plasma::ToolTipManager::self()->show(this);

        m_hideToolTip = startTimer(time);
    }
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
        data.setMainText(m_player->title());
        data.setSubText((m_player->duration() > 0)?i18n("Position: %1 / %2", MetaDataManager::timeToString(m_player->position()), MetaDataManager::timeToString(m_player->duration())):"");
        data.setImage(MetaDataManager::icon(m_player->url()).pixmap(IconSize(KIconLoader::Desktop)));
        data.setAutohide(true);
    }

    Plasma::ToolTipManager::self()->setContent(this, data);
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

void Applet::showMenu(const QPoint &position)
{
    KMenu menu;
    menu.addActions(contextualActions());
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

bool Applet::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::MouseMove && m_fullScreenWidget && m_fullScreenWidget->isFullScreen())
    {
        killTimer(m_hideFullScreenControls);

        m_hideFullScreenControls = startTimer(2000);

        m_fullScreenUi.videoWidget->setCursor(QCursor(Qt::ArrowCursor));
        m_fullScreenUi.titleLabel->show();
        m_fullScreenUi.controlsWidget->show();
    }
    else if (event->type() == QEvent::KeyPress)
    {
        keyPressEvent(static_cast<QKeyEvent*>(event));

        return true;
    }

    return QObject::eventFilter(object, event);
}

}
