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
#include "VideoWidget.h"
#include "MetaDataManager.h"
#include "PlaylistManager.h"
#include "PlaylistModel.h"
#include "PlaylistReader.h"
#include "SeekSlider.h"
#include "PlayerDBusHandler.h"
#include "RootDBusHandler.h"
#include "TrackListDBusHandler.h"

#include <QtCore/QFile>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>
#include <QtGui/QClipboard>
#include <QtGui/QHeaderView>
#include <QtGui/QGraphicsLinearLayout>
#include <QtGui/QGraphicsSceneMouseEvent>
#include <QtGui/QGraphicsSceneResizeEvent>
#include <QtGui/QGraphicsSceneDragDropEvent>
#include <QtDBus/QDBusConnection>

#include <KIcon>
#include <KMenu>
#include <KLocale>
#include <KMessageBox>
#include <KFileDialog>
#include <KInputDialog>
#include <KConfigDialog>

#include <Plasma/Theme>
#include <Plasma/Corona>
#include <Plasma/Slider>
#include <Plasma/ToolButton>
#include <Plasma/Containment>
#include <Plasma/ToolTipManager>

#include <Solid/PowerManagement>

K_EXPORT_PLASMA_APPLET(miniplayer, MiniPlayer::Applet)

namespace MiniPlayer
{

Applet::Applet(QObject *parent, const QVariantList &args) : Plasma::Applet(parent, args),
    m_player(new Player(this)),
    m_videoWidget(new VideoWidget(this)),
    m_volumeDialog(NULL),
    m_playlistDialog(NULL),
    m_playerDBUSHandler(NULL),
    m_trackListDBusHandler(NULL),
    m_rootDBUSHandler(NULL),
    m_controlsWidget(new QGraphicsWidget(this)),
    m_fullScreenWidget(NULL),
    m_jumpToPositionDialog(NULL),
    m_notificationRestrictions(NULL),
    m_hideFullScreenControls(0),
    m_showPlaylist(0),
    m_hideToolTip(0),
    m_editorActive(false),
    m_updateToolTip(false),
    m_videoMode(false)
{
    KGlobal::locale()->insertCatalog("miniplayer");

    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    setAspectRatioMode(Plasma::IgnoreAspectRatio);
    setHasConfigurationInterface(true);
    setAcceptDrops(true);

    QPalette palette = m_videoWidget->palette();
    palette.setColor(QPalette::Window, Qt::black);

    m_videoWidget->setPalette(palette);
    m_videoWidget->setAcceptDrops(true);
    m_videoWidget->setAutoFillBackground(true);
    m_videoWidget->setAcceptHoverEvents(true);
    m_videoWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    m_videoWidget->installEventFilter(this);

    QGraphicsLinearLayout *mainLayout = new QGraphicsLinearLayout(Qt::Vertical, this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->insertItem(0, m_videoWidget);
    mainLayout->insertItem(1, m_controlsWidget);

    setLayout(mainLayout);

    QAction *playlistAction = new QAction(KIcon("view-media-playlist"), i18n("Playlist"), this);
    playlistAction->setShortcut(QKeySequence(Qt::Key_P));

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
    m_actions.append(playlistAction);
    m_actions.append(separator6);

    SeekSlider *seekSlider = new SeekSlider();
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
    volumeButton->setAction(m_player->action(VolumeAction));

    Plasma::ToolButton *playlistButton = new Plasma::ToolButton(m_controlsWidget);
    playlistButton->setAction(playlistAction);

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

    m_playlistDialog = new Plasma::Dialog(NULL, Qt::Tool);
    m_playlistDialog->setFocusPolicy(Qt::NoFocus);
    m_playlistDialog->setWindowTitle(i18n("Playlist"));
    m_playlistDialog->setWindowIcon(KIcon("applications-multimedia"));
    m_playlistDialog->setResizeHandleCorners(Plasma::Dialog::All);

    m_playlistUi.setupUi(m_playlistDialog);

    m_playlistDialog->setContentsMargins(0, 0, 0, 0);
    m_playlistDialog->adjustSize();

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

    if (args.count())
    {
        KUrl::List tracks;

        for (int i = 0; i < args.count(); ++i)
        {
            tracks.append(KUrl(args.value(i).toString()));
        }

        createPlaylist(i18n("Default"), tracks);
    }

    setMinimumWidth(150);
    resize(250, 50);

    connect(this, SIGNAL(activate()), this, SLOT(togglePlaylistDialog()));
    connect(this, SIGNAL(destroyed()), m_playlistDialog, SLOT(deleteLater()));
    connect(m_player, SIGNAL(stateChanged(PlayerState)), this, SLOT(stateChanged(PlayerState)));
    connect(m_player, SIGNAL(videoAvailableChanged(bool)), this, SLOT(videoAvailableChanged(bool)));
    connect(m_player, SIGNAL(metaDataChanged()), this, SLOT(metaDataChanged()));
    connect(m_player, SIGNAL(durationChanged(qint64)), this, SLOT(metaDataChanged()));
    connect(m_player->metaDataManager(), SIGNAL(urlChanged(KUrl)), this, SIGNAL(resetModel()));
    connect(m_playlistDialog, SIGNAL(dialogResized()), this, SLOT(savePlaylistSettings()));
    connect(m_playlistUi.splitter, SIGNAL(splitterMoved(int, int)), this, SLOT(savePlaylistSettings(int, int)));
    connect(m_playlistUi.tabBar, SIGNAL(newTabRequest()), this, SLOT(newPlaylist()));
    connect(m_playlistUi.tabBar, SIGNAL(tabDoubleClicked(int)), this, SLOT(renamePlaylist(int)));
    connect(m_playlistUi.tabBar, SIGNAL(closeRequest(int)), this, SLOT(removePlaylist(int)));
    connect(m_playlistUi.tabBar, SIGNAL(currentChanged(int)), this, SLOT(visiblePlaylistChanged(int)));
    connect(m_playlistUi.tabBar, SIGNAL(tabMoved(int, int)), this, SLOT(movePlaylist(int, int)));
    connect(m_playlistUi.closeButton, SIGNAL(clicked()), m_playlistDialog, SLOT(close()));
    connect(m_playlistUi.addButton, SIGNAL(clicked()), this, SLOT(openFiles()));
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
    connect(m_player->action(OpenFileAction), SIGNAL(triggered()), this, SLOT(openFiles()));
    connect(m_player->action(OpenUrlAction), SIGNAL(triggered()), this, SLOT(openUrl()));
    connect(m_player->action(SeekToAction), SIGNAL(triggered()), this, SLOT(toggleJumpToPosition()));
    connect(m_player->action(FullScreenAction), SIGNAL(triggered()), this, SLOT(toggleFullScreen()));
    connect(m_player->action(VolumeAction), SIGNAL(triggered()), this, SLOT(toggleVolumeDialog()));
    connect(playlistAction, SIGNAL(triggered()), this, SLOT(togglePlaylistDialog()));
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(updateTheme()));
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
    KConfigGroup configuration = config();
    QStringList playlists = configuration.readEntry("playlists", QStringList(i18n("Default")));

    m_visiblePlaylist = configuration.readEntry("currentPlaylist", i18n("Default"));

    m_playlistDialog->resize(configuration.readEntry("playlistSize", m_playlistDialog->size()));

    m_playlistUi.splitter->setStretchFactor(0, 1);
    m_playlistUi.splitter->setStretchFactor(1, 3);
    m_playlistUi.splitter->setStretchFactor(2, 1);
    m_playlistUi.splitter->setStretchFactor(3, 3);
    m_playlistUi.splitter->setStretchFactor(4, 1);
    m_playlistUi.splitter->setStretchFactor(5, 1);
    m_playlistUi.splitter->restoreState(configuration.readEntry("playlistSplitter", QByteArray()));

    KUrl::List tracks;
    QStringList titles;
    QStringList durations;
    QString playlist = m_visiblePlaylist;
    int currentIndex = 0;
    bool playMedia = true;

    for (int i = 0; i < playlists.count(); ++i)
    {
        tracks = KUrl::List(configuration.readEntry("playlistUrls-" + playlists.at(i), QStringList()));
        titles = configuration.readEntry("playlistTitles-" + playlists.at(i), QStringList());
        durations = configuration.readEntry("playlistDurations-" + playlists.at(i), QStringList());

        for (int j = 0; j < tracks.count(); ++j)
        {
            qint64 duration = durations.value(j, "-1").toLongLong();

            if (titles.value(j, QString()).isEmpty() && duration < 1)
            {
                continue;
            }

            m_player->metaDataManager()->setMetaData(tracks.at(j), titles.value(j, QString()), duration);
        }

        createPlaylist(playlists.at(i), tracks);

        if (playlists.at(i) == playlist)
        {
            currentIndex = i;
        }
    }

    setCurrentPlaylist(playlist);

    m_playlistUi.tabBar->setCurrentIndex(currentIndex);

    if (m_playlistUi.tabBar->count() == 1)
    {
        m_playlistUi.tabBar->hide();
    }

    if (!configuration.readEntry("playOnStartup", false))
    {
        playMedia = false;
    }

//     m_player->setVideoOutput(m_videoWidget->videoItem());
    m_player->setVolume(configuration.readEntry("volume", 50));
    m_player->setAudioMuted(configuration.readEntry("muted", false));

    if (m_playlists[m_visiblePlaylist]->playlist()->mediaCount())
    {
        KUrl currentTrack;

        currentTrack = configuration.readEntry("currentTrack", KUrl());

        savePlaylist();

        if (playMedia)
        {
            if (!currentTrack.isValid())
            {
                currentTrack = KUrl(m_playlists[m_visiblePlaylist]->playlist()->media(0).canonicalUrl());
            }

            play(currentTrack);
        }
    }

    videoAvailableChanged(false);
    updateTheme();

    QTimer::singleShot(100, this, SLOT(configReset()));
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
    connect(parent, SIGNAL(finished()), this, SLOT(configReset()));
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

    m_videoMode = (!visible|| (size().height() - m_controlsWidget->size().height()) > 50);

    updateVideoWidgets();

    m_player->setPlaybackMode(static_cast<PlaybackMode>(configuration.readEntry("playbackMode", static_cast<int>(LoopPlaylistMode))));
    m_player->setAspectRatio(static_cast<AspectRatio>(configuration.readEntry("ratio", static_cast<int>(AutomaticRatio))));
    m_player->setAudioMuted(configuration.readEntry("mute", false));
    m_player->setVolume(configuration.readEntry("volume", 50));


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
}

void Applet::constraintsEvent(Plasma::Constraints constraints)
{
    if ((constraints & Plasma::LocationConstraint || constraints & Plasma::SizeConstraint) && m_playlistDialog->isVisible())
    {
        m_playlistDialog->move(popupPosition(m_playlistDialog->size()));
    }
}

void Applet::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    m_videoMode = (!m_controlsWidget->isVisible() || (event->newSize().height() - m_controlsWidget->size().height()) > 50);

    Plasma::Applet::resizeEvent(event);

    updateVideoWidgets();
}

void Applet::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    if (!event->mimeData()->hasUrls())
    {
        event->ignore();

        return;
    }

    if (!m_playlistDialog->isVisible())
    {
        m_showPlaylist = startTimer(500);
    }
}

void Applet::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
{
    Q_UNUSED(event)

    killTimer(m_showPlaylist);
}

void Applet::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    killTimer(m_showPlaylist);

    m_playlists[m_visiblePlaylist]->addTracks(KUrl::List::fromMimeData(event->mimeData()), -1, true);
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
            else if (m_playlistDialog->isVisible())
            {
                m_playlistDialog->close();
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
    }
    else if (event->timerId() == m_showPlaylist)
    {
        togglePlaylistDialog();
    }
    else if (event->timerId() == m_hideToolTip)
    {
        if (!isUnderMouse())
        {
            Plasma::ToolTipManager::self()->hide(this);
        }

        m_hideToolTip = 0;
    }

    killTimer(event->timerId());
}

void Applet::stateChanged(PlayerState state)
{
    if (m_player->isVideoAvailable() && state == PlayingState)
    {
        m_stopSleepCookie = Solid::PowerManagement::beginSuppressingSleep("Plasma MiniPlayerApplet: playing video");

        if (!m_notificationRestrictions)
        {
            m_notificationRestrictions = new KNotificationRestrictions(KNotificationRestrictions::NonCriticalServices, this);
        }
    }
    else if (m_notificationRestrictions)
    {
        Solid::PowerManagement::stopSuppressingSleep(m_stopSleepCookie);

        delete m_notificationRestrictions;

        m_notificationRestrictions = NULL;
    }

    if (state == StoppedState)
    {
        m_playlistUi.titleLabel->setText(QString());

        if (m_fullScreenWidget)
        {
            m_fullScreenUi.titleLabel->setText(QString());
        }

        m_playlistUi.titleLabel->setText(QString());

        if (m_fullScreenWidget)
        {
            m_fullScreenUi.titleLabel->setText(QString());
        }

        Plasma::ToolTipManager::self()->clearContent(this);

        videoAvailableChanged(false);
    }
}

void Applet::videoAvailableChanged(bool videoAvailable)
{
///FIXME switcher for signal in videoWidget (label)
//     m_videoWidget->update();

    if (!videoAvailable && m_fullScreenWidget && m_fullScreenWidget->isFullScreen())
    {
        toggleFullScreen();
    }
}

void Applet::metaDataChanged()
{
    QString title = m_player->title();
    KUrl url = m_player->url();

    if (!title.isEmpty())
    {
        m_title = title;

        if (!m_player->metaDataManager()->available(url))
        {
            m_player->metaDataManager()->setMetaData(url, m_title, m_player->duration());

            emit resetModel();

            savePlaylist();
        }
    }
    else
    {
        m_title = m_player->metaDataManager()->title(url);
    }

    m_playlistUi.titleLabel->setText(m_title);

    if (m_fullScreenWidget)
    {
        m_fullScreenUi.titleLabel->setText(m_title);
    }

    if (m_player->position() < 3 && m_hideToolTip == 0)
    {
        updateToolTip();

        QTimer::singleShot(500, this, SLOT(showToolTip()));
    }
}

void Applet::trackPressed()
{
    if (!m_playlistUi.playlistView->selectionModel())
    {
        return;
    }

    QModelIndexList selectedIndexes = m_playlistUi.playlistView->selectionModel()->selectedIndexes();
    bool hasTracks = m_playlists[m_visiblePlaylist]->playlist()->mediaCount();

    m_playlistUi.addButton->setEnabled(!m_playlists[m_visiblePlaylist]->playlist()->isReadOnly());
    m_playlistUi.removeButton->setEnabled(!selectedIndexes.isEmpty());
    m_playlistUi.editButton->setEnabled(!selectedIndexes.isEmpty() && !m_playlists[m_visiblePlaylist]->playlist()->isReadOnly());
    m_playlistUi.moveUpButton->setEnabled((m_playlists[m_visiblePlaylist]->playlist()->mediaCount() > 1) && !selectedIndexes.isEmpty() && selectedIndexes.first().row() != 0);
    m_playlistUi.moveDownButton->setEnabled((m_playlists[m_visiblePlaylist]->playlist()->mediaCount() > 1) && !selectedIndexes.isEmpty() && selectedIndexes.last().row() != (m_playlists[m_visiblePlaylist]->playlist()->mediaCount() - 1));
    m_playlistUi.clearButton->setEnabled(hasTracks && !m_playlists[m_visiblePlaylist]->playlist()->isReadOnly());
    m_playlistUi.playbackModeButton->setEnabled(hasTracks);
    m_playlistUi.exportButton->setEnabled(hasTracks && !m_playlists[m_visiblePlaylist]->playlist()->isReadOnly());
    m_playlistUi.shuffleButton->setEnabled((m_playlists[m_visiblePlaylist]->playlist()->mediaCount() > 1) && !m_playlists[m_visiblePlaylist]->playlist()->isReadOnly());
    m_playlistUi.playlistViewFilter->setEnabled(hasTracks);

    m_editorActive = false;
}

void Applet::trackChanged()
{
    m_editorActive = false;
}

void Applet::moveUpTrack()
{
    KUrl url(m_playlistUi.playlistView->currentIndex().data(Qt::ToolTipRole).toString());
    int row = m_playlistUi.playlistView->currentIndex().row();

    m_playlists[m_visiblePlaylist]->playlist()->removeMedia(row);
    m_playlists[m_visiblePlaylist]->playlist()->insertMedia((row - 1), QMediaContent(url));

    m_playlistUi.playlistView->setCurrentIndex(m_playlists[m_visiblePlaylist]->index((row - 1), 0));

    trackPressed();
}

void Applet::moveDownTrack()
{
    KUrl url(m_playlistUi.playlistView->currentIndex().data(Qt::ToolTipRole).toString());
    int row = m_playlistUi.playlistView->currentIndex().row();

    m_playlists[m_visiblePlaylist]->playlist()->removeMedia(row);
    m_playlists[m_visiblePlaylist]->playlist()->insertMedia((row + 1), QMediaContent(url));

    m_playlistUi.playlistView->setCurrentIndex(m_playlists[m_visiblePlaylist]->index((row + 1), 0));

    trackPressed();
}

void Applet::playTrack(QModelIndex index)
{
    if (!index.isValid())
    {
        index = m_playlistUi.playlistView->currentIndex();
    }

    if (m_visiblePlaylist != m_currentPlaylist)
    {
        setCurrentPlaylist(m_visiblePlaylist);
    }

    play(index.row());
}

void Applet::editTrackTitle()
{
    m_editorActive = true;

    m_playlistUi.playlistView->edit(m_playlistUi.playlistView->currentIndex());
}

void Applet::copyTrackUrl()
{
    QApplication::clipboard()->setText(m_playlistUi.playlistView->currentIndex().data(Qt::ToolTipRole).toString());
}

void Applet::removeTrack()
{
    int row = m_playlistUi.playlistView->currentIndex().row();

    m_playlists[m_visiblePlaylist]->playlist()->removeMedia(row);

    m_playlistUi.playlistView->setCurrentIndex(m_playlists[m_visiblePlaylist]->index(row, 0));
}

void Applet::openUrl()
{
    m_playlists[m_visiblePlaylist]->addTracks(KUrl::List(KInputDialog::getText(i18n("Open URL"), i18n("Enter a URL:"))), -1, true);
}

void Applet::openFiles()
{
    KConfigGroup configuration = config();

    KFileDialog dialog(KUrl(configuration.readEntry("directory", "~")), "", NULL);
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

    configuration.writeEntry("directory", QFileInfo(urls.at(0).toLocalFile()).absolutePath());

    m_playlists[m_visiblePlaylist]->addTracks(urls, -1, (sender() == m_player->action(OpenFileAction)));

    emit configNeedsSaving();
}

void Applet::play(int index)
{
    m_playlists[m_visiblePlaylist]->playlist()->setCurrentIndex(index);

    m_player->play();

    QTimer::singleShot(500, this, SLOT(showToolTip()));
}

void Applet::play(KUrl url)
{
    if (!url.isValid())
    {
        return;
    }

    int index = -1;

///FIXME use m_player->playlist()?
    for (int i = 0; i < m_playlists[m_currentPlaylist]->playlist()->mediaCount(); ++i)
    {
        if (url == KUrl(m_playlists[m_currentPlaylist]->playlist()->media(i).canonicalUrl()))
        {
            index = i;

            break;
        }
    }

    config().writeEntry("currentTrack", url);

    emit configNeedsSaving();

    if (index < 0)
    {
        m_playlists[m_visiblePlaylist]->addTracks(KUrl::List(url), -1, true);

        return;
    }

    play(index);
}

void Applet::movePlaylist(int from, int to)
{
    Q_UNUSED(from)
    Q_UNUSED(to)

    savePlaylistNames();
}

void Applet::renamePlaylist(int position)
{
    QString oldName = KGlobal::locale()->removeAcceleratorMarker(m_playlistUi.tabBar->tabText(position));
    QString newName = KInputDialog::getText(i18n("Rename playlist"), i18n("Enter name:"), oldName);

    if (newName.isEmpty() || m_playlists.contains(newName))
    {
        return;
    }

    KConfigGroup configuration = config();
    configuration.writeEntry("currentPlaylist", newName);
    configuration.writeEntry("playlistUrls-" + newName, configuration.readEntry("playlistUrls-" + oldName, QStringList()));
    configuration.writeEntry("playlistTitles-" + newName, configuration.readEntry("playlistTitles-" + oldName, QStringList()));
    configuration.deleteEntry("playlistUrls-" + oldName);
    configuration.deleteEntry("playlistTitles-" + oldName);

    m_playlists[newName] = m_playlists[oldName];
    m_playlists.remove(oldName);

    if (m_currentPlaylist == oldName)
    {
        setCurrentPlaylist(newName);
    }

    m_playlistUi.tabBar->setTabText(position, newName);

    savePlaylistNames();

    emit configNeedsSaving();
}

void Applet::removePlaylist(int position)
{
    if (m_playlistUi.tabBar->count() == 1)
    {
        clearPlaylist();

        return;
    }

    QString oldName = KGlobal::locale()->removeAcceleratorMarker(m_playlistUi.tabBar->tabText(position));

    m_playlistUi.tabBar->removeTab(position);

    QString newName = KGlobal::locale()->removeAcceleratorMarker(m_playlistUi.tabBar->tabText(m_playlistUi.tabBar->currentIndex()));

    m_playlists.remove(oldName);

    KConfigGroup configuration = config();
    configuration.writeEntry("currentPlaylist", newName);
    configuration.deleteEntry("playlistUrls-" + oldName);
    configuration.deleteEntry("playlistTitles-" + oldName);

    if (m_currentPlaylist == oldName)
    {
        setCurrentPlaylist(newName);
    }

    if (m_playlistUi.tabBar->count() == 1)
    {
        m_playlistUi.tabBar->hide();
    }

    savePlaylistNames();

    emit configNeedsSaving();
}

void Applet::visiblePlaylistChanged(int position)
{
    QString name = KGlobal::locale()->removeAcceleratorMarker(m_playlistUi.tabBar->tabText(position));

    if (!m_playlists.contains(name))
    {
        return;
    }

    m_visiblePlaylist = name;

    filterPlaylist(m_playlistUi.playlistViewFilter->text());

    if (m_player->state() != PlayingState && m_player->state() != PausedState)
    {
        setCurrentPlaylist(KGlobal::locale()->removeAcceleratorMarker(m_playlistUi.tabBar->tabText(position)));
    }

    m_playlistUi.playlistView->setModel(m_playlists[m_visiblePlaylist]);
    m_playlistUi.playlistView->horizontalHeader()->setResizeMode(0, QHeaderView::Fixed);
    m_playlistUi.playlistView->horizontalHeader()->resizeSection(0, 22);
    m_playlistUi.playlistView->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);

    if (!m_playlistUi.playlistView->horizontalHeader()->restoreState(config().readEntry("playlistViewHeader", QByteArray())))
    {
        m_playlistUi.playlistView->horizontalHeader()->resizeSection(1, 300);
    }

    savePlaylistNames();
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
        m_jumpToPositionDialog = new KDialog(m_playlistDialog);
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

void Applet::togglePlaylistDialog()
{
    if (m_playlistDialog->isVisible())
    {
        m_playlistDialog->close();
    }
    else
    {
        m_playlistDialog->move(containment()->corona()->popupPosition(this, m_playlistDialog->size(), Qt::AlignCenter));

        trackPressed();

        m_playlistDialog->show();
    }
}

void Applet::toggleFullScreen()
{
    if (!m_fullScreenWidget)
    {
        m_fullScreenWidget = new QWidget;

        m_fullScreenUi.setupUi(m_fullScreenWidget);
        m_fullScreenUi.playPauseButton->setDefaultAction(m_player->action(PlayPauseAction));
        m_fullScreenUi.stopButton->setDefaultAction(m_player->action(StopAction));
        m_fullScreenUi.seekSlider->setPlayer(m_player);
        m_fullScreenUi.muteButton->setDefaultAction(m_player->action(MuteAction));
        m_fullScreenUi.volumeSlider->setPlayer(m_player);
        m_fullScreenUi.fullScreenButton->setDefaultAction(m_player->action(FullScreenAction));
        m_fullScreenUi.videoOutputWidget->installEventFilter(this);

        connect(this, SIGNAL(destroyed()), m_fullScreenWidget, SLOT(deleteLater()));
    }

    if (m_fullScreenWidget->isFullScreen() || !m_player->isVideoAvailable())
    {
        killTimer(m_hideFullScreenControls);

        m_fullScreenWidget->showNormal();
        m_fullScreenWidget->hide();

        m_player->action(FullScreenAction)->setIcon(KIcon("view-fullscreen"));
        m_player->action(FullScreenAction)->setText(i18n("Full Screen Mode"));

        m_fullScreenUi.videoWidget->setCursor(QCursor(Qt::ArrowCursor));

        updateVideoWidgets();
    }
    else
    {
        Plasma::ToolTipManager::self()->hide(this);

//        m_player->setVideoOutput(m_fullScreenUi.videoWidget);

        m_fullScreenWidget->showFullScreen();
        m_fullScreenWidget->setWindowTitle(m_title);

        m_fullScreenUi.titleLabel->setText(m_title);
        m_fullScreenUi.titleLabel->hide();
        m_fullScreenUi.controlsWidget->hide();

        m_player->action(FullScreenAction)->setIcon(KIcon("view-restore"));
        m_player->action(FullScreenAction)->setText(i18n("Exit Full Screen Mode"));

        m_hideFullScreenControls = startTimer(2000);
    }
}

void Applet::updateVideoWidgets()
{
    m_videoWidget->setVisible(m_videoMode);

    if (m_fullScreenWidget && m_fullScreenWidget->isFullScreen())
    {
        return;
    }

    m_playlistUi.blankLabel->hide();
    m_playlistUi.videoWidget->hide();

    if (m_videoMode)
    {
//        m_player->setVideoOutput(m_videoWidget->videoItem());

        m_playlistUi.blankLabel->show();
    }
    else
    {
//        m_player->setVideoOutput(m_playlistUi.videoWidget);

        m_playlistUi.videoWidget->show();
    }
}

void Applet::filterPlaylist(const QString &text)
{
    for (int i = 0; i < m_playlists[m_visiblePlaylist]->playlist()->mediaCount(); ++i)
    {
        m_playlistUi.playlistView->setRowHidden(i, (!text.isEmpty() && !m_playlists[m_visiblePlaylist]->index(i, 1).data(Qt::DisplayRole).toString().contains(text, Qt::CaseInsensitive)));
    }
}

void Applet::savePlaylistNames()
{
///FIXME use iterator on QHash?
    QStringList names;
    QString name;

    for (int i = 0; i < m_playlistUi.tabBar->count(); ++i)
    {
        names.append(KGlobal::locale()->removeAcceleratorMarker(m_playlistUi.tabBar->tabText(i)));
    }

    config().writeEntry("playlists", names);

    emit configNeedsSaving();
}

void Applet::savePlaylist()
{
    KConfigGroup configuration = config();
    QStringList urls;
    QStringList titles;
    QStringList durations;

    for (int i = 0; i < m_playlists[m_visiblePlaylist]->playlist()->mediaCount(); ++i)
    {
        KUrl url(m_playlists[m_visiblePlaylist]->playlist()->media(i).canonicalUrl());

        if (m_player->metaDataManager()->available(url))
        {
            titles.append(m_player->metaDataManager()->title(url));
            durations.append(QString::number(m_player->metaDataManager()->duration(url)));
        }
        else
        {
            titles.append(QString());
            durations.append(QString("-1"));
        }

        urls.append(url.pathOrUrl());
    }

    configuration.writeEntry("playlistUrls-" + m_visiblePlaylist, urls);
    configuration.writeEntry("playlistTitles-" + m_visiblePlaylist, titles);
    configuration.writeEntry("playlistDurations-" + m_visiblePlaylist, durations);

    emit configNeedsSaving();
}

void Applet::createPlaylist(const QString &playlist, const KUrl::List &tracks)
{
    if (m_playlists.contains(playlist) || playlist.isEmpty())
    {
        return;
    }

    m_playlists[playlist] = new PlaylistModel(m_player);

    if (!tracks.isEmpty())
    {
        m_playlists[playlist]->addTracks(tracks);

        m_player->metaDataManager()->addTracks(tracks);
    }

    m_playlistUi.tabBar->show();

    int position = m_playlistUi.tabBar->currentIndex();

    if (position == (m_playlistUi.tabBar->count() - 1))
    {
        m_playlistUi.tabBar->addTab(KIcon("view-media-playlist"), playlist);
    }
    else
    {
        m_playlistUi.tabBar->insertTab((position + 1), KIcon("view-media-playlist"), playlist);
    }

    m_playlistUi.tabBar->setCurrentIndex(position + 1);

    savePlaylistNames();

    connect(m_playlists[playlist], SIGNAL(needsSaving()), this, SLOT(savePlaylist()));
    connect(m_playlists[playlist], SIGNAL(itemChanged(QModelIndex)), this, SLOT(trackChanged()));
}

void Applet::exportPlaylist()
{
    KFileDialog dialog(KUrl(config().readEntry("directory", "~")), QString(), NULL);
    dialog.setMimeFilter(QStringList() << "audio/x-scpls" << "audio/x-mpegurl");
    dialog.setWindowModality(Qt::NonModal);
    dialog.setMode(KFile::File);
    dialog.setOperationMode(KFileDialog::Saving);
    dialog.setConfirmOverwrite(true);
    dialog.setSelection(m_playlistUi.tabBar->tabText(m_playlistUi.tabBar->currentIndex()).remove('&') + ".pls");
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
            out << "NumberOfEntries=" << m_playlists[m_visiblePlaylist]->playlist()->mediaCount() << "\n\n";
        }
        else
        {
            out << "#EXTM3U\n\n";
        }

        for (int i = 0; i < m_playlists[m_visiblePlaylist]->playlist()->mediaCount(); ++i)
        {
            available = m_player->metaDataManager()->available(url);
            url = KUrl(m_playlists[m_visiblePlaylist]->playlist()->media(i).canonicalUrl());
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

void Applet::newPlaylist()
{
    createPlaylist(KInputDialog::getText(i18n("New playlist"), i18n("Enter name:")));
}

void Applet::clearPlaylist()
{
    m_playlists[m_visiblePlaylist]->playlist()->clear();
}

void Applet::shufflePlaylist()
{
    m_playlists[m_visiblePlaylist]->playlist()->shuffle();
}

void Applet::setCurrentPlaylist(const QString &playlist)
{
    QString name;

    m_currentPlaylist = playlist;

    for (int i = 0; i < m_playlistUi.tabBar->count(); ++i)
    {
        name = KGlobal::locale()->removeAcceleratorMarker(m_playlistUi.tabBar->tabText(i));

        m_playlistUi.tabBar->setTabIcon(i, ((playlist == name)?KIcon("media-playback-start"):KIcon("view-media-playlist")));
    }

    m_player->setPlaylist(m_playlists[m_currentPlaylist]->playlist());

    config().writeEntry("currentPlaylist", playlist);

    emit configNeedsSaving();
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

    if (m_player->state() == PlayingState || m_player->state() == PausedState)
    {
        data.setMainText(m_title);
        data.setSubText((m_player->duration() > 0)?i18n("Position: %1 / %2", MetaDataManager::timeToString(m_player->position()), MetaDataManager::timeToString(m_player->duration())):"");
        data.setImage(m_player->metaDataManager()->icon(m_player->url()).pixmap(IconSize(KIconLoader::Desktop)));
        data.setAutohide(true);
    }

    Plasma::ToolTipManager::self()->setContent(this, data);
}

void Applet::toolTipAboutToShow()
{
    m_updateToolTip = true;

    updateToolTip();

    m_updateToolTip = false;
}

void Applet::toolTipHidden()
{
    Plasma::ToolTipManager::self()->clearContent(this);
}

void Applet::savePlaylistSettings(int position, int index)
{
    Q_UNUSED(position)
    Q_UNUSED(index)

    KConfigGroup configuration = config();

    configuration.writeEntry("playlistSize", m_playlistDialog->size());
    configuration.writeEntry("playlistSplitter", m_playlistUi.splitter->saveState());
    configuration.writeEntry("playlistViewHeader", m_playlistUi.playlistView->horizontalHeader()->saveState());

    emit configNeedsSaving();
}

void Applet::updateTheme()
{
//     QPalette palette = m_playlistDialog->palette();
//     palette.setColor(QPalette::WindowText, Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor));
//     palette.setColor(QPalette::ButtonText, Plasma::Theme::defaultTheme()->color(Plasma::Theme::ButtonTextColor));
//     palette.setColor(QPalette::Background, Plasma::Theme::defaultTheme()->color(Plasma::Theme::BackgroundColor));
//     palette.setColor(QPalette::Button, palette.color(QPalette::Background).lighter(250));
//
//     m_playlistDialog->setPalette(palette);
}

QList<QAction*> Applet::contextualActions()
{
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

        return true;
    }
    else if (object == m_videoWidget || object == m_playlistUi.videoOutputWidget || object == m_fullScreenUi.videoOutputWidget)
    {
        if (event->type() == QEvent::ContextMenu)
        {
            KMenu menu;
            menu.addActions(m_actions);
            menu.addSeparator();
            menu.addAction(KIcon("configure"), i18n("Settings"), this, SLOT(showConfigurationInterface()));
            menu.exec(QCursor::pos());

            return true;
        }
        else if ((event->type() == QEvent::MouseButtonDblClick && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) || (event->type() == QEvent::GraphicsSceneMouseDoubleClick && static_cast<QGraphicsSceneMouseEvent*>(event)->button() == Qt::LeftButton))
        {
            toggleFullScreen();

            return true;
        }
        else if (event->type() == QEvent::KeyPress)
        {
            keyPressEvent(static_cast<QKeyEvent*>(event));

            return true;
        }
        else if (event->type() == QEvent::DragEnter)
        {
            static_cast<QDragEnterEvent*>(event)->acceptProposedAction();

            return true;
        }
        else if (event->type() == QEvent::Drop)
        {
            m_playlists[m_visiblePlaylist]->addTracks(KUrl::List::fromMimeData(static_cast<QDropEvent*>(event)->mimeData()), -1, true);

            return true;
        }
    }
    else if (object == m_playlistUi.playlistView)
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
                keyPressEvent(keyEvent);
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
    }

    return QObject::eventFilter(object, event);
}

}
