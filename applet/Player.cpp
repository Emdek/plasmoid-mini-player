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

#include "Player.h"
#include "MetaDataManager.h"
#include "PlaylistModel.h"
#include "VideoWidget.h"

#include <QtGui/QLayout>
#include <QtGui/QWidgetAction>
#include <QtGui/QGraphicsSceneMouseEvent>

#include <QGlib/Error>
#include <QGlib/Connect>

#include <QGst/Bus>
#include <QGst/Init>
#include <QGst/Event>
#include <QGst/Query>
#include <QGst/TagList>
#include <QGst/ClockTime>
#include <QGst/ElementFactory>

#include <KIcon>
#include <KMenu>
#include <KLocale>
#include <KMessageBox>

#include <Solid/PowerManagement>

namespace MiniPlayer
{

Player::Player(QObject *parent) : QObject(parent),
    m_notificationRestrictions(NULL),
    m_appletVideoWidget(NULL),
    m_dialogVideoWidget(NULL),
    m_fullScreenWidget(NULL),
    m_chaptersGroup(new QActionGroup(this)),
    m_audioChannelGroup(new QActionGroup(this)),
    m_subtitlesGroup(new QActionGroup(this)),
    m_anglesGroup(new QActionGroup(this)),
    m_aspectRatio(AutomaticRatio),
    m_stopSleepCookie(0),
    m_hideFullScreenControlsTimer(0),
    m_playPauseTimer(0),
    m_inhibitNotifications(false),
    m_videoMode(false)
{
    QGst::init();
//     m_videoWidget->setScaleMode(Phonon::VideoWidget::FitInView);
//     m_videoWidget->setWindowIcon(KIcon("applications-multimedia"));
//     m_videoWidget->setAcceptDrops(true);

    m_actions[OpenMenuAction] = new QAction(i18n("Open"), this);
    m_actions[OpenMenuAction]->setMenu(new KMenu());
    m_actions[OpenFileAction] = m_actions[OpenMenuAction]->menu()->addAction(KIcon("document-open"), i18n("Open File..."));
    m_actions[OpenFileAction]->setShortcut(QKeySequence(Qt::Key_O));
    m_actions[OpenUrlAction] = m_actions[OpenMenuAction]->menu()->addAction(KIcon("uri-mms"), i18n("Open URL..."));
    m_actions[OpenUrlAction]->setShortcut(QKeySequence(Qt::Key_U));

    m_actions[PlayPauseAction] = new QAction(KIcon("media-playback-start"), i18n("Play"), this);
    m_actions[PlayPauseAction]->setEnabled(false);
    m_actions[PlayPauseAction]->setShortcut(QKeySequence(Qt::Key_Space));
    m_actions[StopAction] = new QAction(KIcon("media-playback-stop"), i18n("Stop"), this);
    m_actions[StopAction]->setEnabled(false);
    m_actions[StopAction]->setShortcut(QKeySequence(Qt::Key_S));
    m_actions[PlaylistToggleAction] = new QAction(KIcon("view-media-playlist"), i18n("Playlist"), this);
    m_actions[PlaylistToggleAction]->setShortcut(QKeySequence(Qt::Key_P));

    m_actions[NavigationMenuAction] = new QAction(i18n("Navigation"), this);
    m_actions[NavigationMenuAction]->setMenu(new KMenu());
    m_actions[NavigationMenuAction]->setEnabled(false);
    m_actions[ChapterMenuAction] = m_actions[NavigationMenuAction]->menu()->addAction(i18n("Chapter"));
    m_actions[ChapterMenuAction]->setMenu(new KMenu());
    m_actions[ChapterMenuAction]->setEnabled(false);
    m_actions[PlayPreviousAction] = m_actions[NavigationMenuAction]->menu()->addAction(KIcon("media-skip-backward"), i18n("Previous"), this, SLOT(playPrevious()), QKeySequence(Qt::Key_PageDown));
    m_actions[PlayNextAction] = m_actions[NavigationMenuAction]->menu()->addAction(KIcon("media-skip-forward"), i18n("Next"), this, SLOT(playNext()), QKeySequence(Qt::Key_PageUp));
    m_actions[NavigationMenuAction]->menu()->addSeparator();
    m_actions[SeekBackwardAction] = m_actions[NavigationMenuAction]->menu()->addAction(KIcon("media-seek-backward"), i18n("Seek Backward"), this, SLOT(seekBackward()), QKeySequence(Qt::Key_Left));
    m_actions[SeekForwardAction] = m_actions[NavigationMenuAction]->menu()->addAction(KIcon("media-seek-forward"), i18n("Seek Forward"), this, SLOT(seekForward()), QKeySequence(Qt::Key_Right));
    m_actions[NavigationMenuAction]->menu()->addSeparator();
    m_actions[SeekToAction] = m_actions[NavigationMenuAction]->menu()->addAction(KIcon("go-jump"), i18n("Jump to Position..."));
    m_actions[SeekToAction]->setShortcut(QKeySequence(Qt::Key_G));

    m_actions[VolumeToggleAction] = new QAction(KIcon("player-volume"), i18n("Volume"), this);
    m_actions[VolumeToggleAction]->setShortcut(QKeySequence(Qt::Key_V));
    m_actions[AudioMenuAction] = new QAction(i18n("Audio"), this);
    m_actions[AudioMenuAction]->setMenu(new KMenu());
    m_actions[AudioChannelMenuAction] = m_actions[AudioMenuAction]->menu()->addAction(i18n("Channel"));
    m_actions[AudioChannelMenuAction]->setMenu(new KMenu());
    m_actions[AudioChannelMenuAction]->setEnabled(false);
    m_actions[IncreaseVolumeAction] = m_actions[AudioMenuAction]->menu()->addAction(KIcon("audio-volume-high"), i18n("Increase Volume"), this, SLOT(increaseVolume()), QKeySequence(Qt::Key_Plus));
    m_actions[DecreaseVolumeAction] = m_actions[AudioMenuAction]->menu()->addAction(KIcon("audio-volume-low"), i18n("Decrease Volume"), this, SLOT(decreaseVolume()), QKeySequence(Qt::Key_Minus));
    m_actions[AudioMenuAction]->menu()->addSeparator();
    m_actions[MuteAction] = m_actions[AudioMenuAction]->menu()->addAction(KIcon("audio-volume-medium"), i18n("Mute Volume"));
    m_actions[MuteAction]->setCheckable(true);
    m_actions[MuteAction]->setShortcut(QKeySequence(Qt::Key_M));

    m_actions[VideoMenuAction] = new QAction(i18n("Video"), this);
    m_actions[VideoMenuAction]->setMenu(new KMenu());
    m_actions[VideoMenuAction]->setEnabled(false);
    m_actions[VideoPropepertiesMenu] = m_actions[VideoMenuAction]->menu()->addAction(i18n("Properties"));
    m_actions[VideoPropepertiesMenu]->setMenu(new KMenu());

    m_brightnessSlider = new QSlider;
    m_brightnessSlider->setOrientation(Qt::Horizontal);
    m_brightnessSlider->setRange(0, 100);
    m_brightnessSlider->setValue(50);
    m_brightnessSlider->setMinimumWidth(150);

    QWidgetAction *brightnessAction = new QWidgetAction(this);
    brightnessAction->setDefaultWidget(m_brightnessSlider);

    m_contrastSlider = new QSlider;
    m_contrastSlider->setOrientation(Qt::Horizontal);
    m_contrastSlider->setRange(0, 100);
    m_contrastSlider->setValue(50);
    m_contrastSlider->setMinimumWidth(150);

    QWidgetAction *contrastAction = new QWidgetAction(this);
    contrastAction->setDefaultWidget(m_contrastSlider);

    m_hueSlider = new QSlider;
    m_hueSlider->setOrientation(Qt::Horizontal);
    m_hueSlider->setRange(0, 100);
    m_hueSlider->setValue(50);
    m_hueSlider->setMinimumWidth(150);

    QWidgetAction *hueAction = new QWidgetAction(this);
    hueAction->setDefaultWidget(m_hueSlider);

    m_saturationSlider = new QSlider;
    m_saturationSlider->setOrientation(Qt::Horizontal);
    m_saturationSlider->setRange(0, 100);
    m_saturationSlider->setValue(50);
    m_saturationSlider->setMinimumWidth(150);

    QWidgetAction *saturationAction = new QWidgetAction(this);
    saturationAction->setDefaultWidget(m_saturationSlider);

    m_actions[VideoPropepertiesMenu]->menu()->addAction(brightnessAction);
    m_actions[VideoPropepertiesMenu]->menu()->addAction(contrastAction);
    m_actions[VideoPropepertiesMenu]->menu()->addAction(hueAction);
    m_actions[VideoPropepertiesMenu]->menu()->addAction(saturationAction);

    m_actions[AspectRatioMenuAction] = m_actions[VideoMenuAction]->menu()->addAction(i18n("Aspect Ratio"));
    m_actions[AspectRatioMenuAction]->setMenu(new KMenu());

    QAction *aspectRatioAutomatic = m_actions[AspectRatioMenuAction]->menu()->addAction(i18n("Automatic"));
    aspectRatioAutomatic->setCheckable(true);
    aspectRatioAutomatic->setData(AutomaticRatio);

    QAction *aspectRatio4_3 = m_actions[AspectRatioMenuAction]->menu()->addAction(i18n("4:3"));
    aspectRatio4_3->setCheckable(true);
    aspectRatio4_3->setData(Ratio4_3);

    QAction *aspectRatio16_9 = m_actions[AspectRatioMenuAction]->menu()->addAction(i18n("16:9"));
    aspectRatio16_9->setCheckable(true);
    aspectRatio16_9->setData(Ratio16_9);

    QAction *aspectRatioFitTo = m_actions[AspectRatioMenuAction]->menu()->addAction(i18n("Fit to Window"));
    aspectRatioFitTo->setCheckable(true);
    aspectRatioFitTo->setData(FitToRatio);

    QActionGroup *aspectRatioActionGroup = new QActionGroup(m_actions[VideoMenuAction]->menu());
    aspectRatioActionGroup->addAction(aspectRatioAutomatic);
    aspectRatioActionGroup->addAction(aspectRatio4_3);
    aspectRatioActionGroup->addAction(aspectRatio16_9);
    aspectRatioActionGroup->addAction(aspectRatioFitTo);

    m_actions[SubtitleMenuAction] = m_actions[VideoMenuAction]->menu()->addAction(i18n("Subtitle"));
    m_actions[SubtitleMenuAction]->setMenu(new KMenu());
    m_actions[SubtitleMenuAction]->setEnabled(false);
    m_actions[AngleMenuAction] = m_actions[VideoMenuAction]->menu()->addAction(i18n("Angle"));
    m_actions[AngleMenuAction]->setMenu(new KMenu());
    m_actions[AngleMenuAction]->setEnabled(false);
    m_actions[VideoMenuAction]->menu()->addSeparator();
    m_actions[FullScreenAction] = m_actions[VideoMenuAction]->menu()->addAction(KIcon("view-fullscreen"), i18n("Full Screen Mode"));
    m_actions[FullScreenAction]->setEnabled(false);
    m_actions[FullScreenAction]->setShortcut(QKeySequence(Qt::Key_F));

    m_actions[PlaybackModeMenuAction] = new QAction(KIcon("edit-redo"), i18n("Playback Mode"), this);
    m_actions[PlaybackModeMenuAction]->setMenu(new KMenu());

    QAction *noRepeatAction = m_actions[PlaybackModeMenuAction]->menu()->addAction(KIcon("dialog-cancel"), i18n("No Repeat"));
    noRepeatAction->setCheckable(true);
    noRepeatAction->setData(SequentialMode);

    QAction *currentTrackOnceAction = m_actions[PlaybackModeMenuAction]->menu()->addAction(KIcon("audio-x-generic"), i18n("Current Track Once"));
    currentTrackOnceAction->setCheckable(true);
    currentTrackOnceAction->setData(CurrentTrackOnceMode);

    QAction *repeatTrackAction = m_actions[PlaybackModeMenuAction]->menu()->addAction(KIcon("audio-x-generic"), i18n("Repeat Track"));
    repeatTrackAction->setCheckable(true);
    repeatTrackAction->setData(LoopTrackMode);

    QAction *repeatPlaylistAction = m_actions[PlaybackModeMenuAction]->menu()->addAction(KIcon("view-media-playlist"), i18n("Repeat Playlist"));
    repeatPlaylistAction->setCheckable(true);
    repeatPlaylistAction->setData(LoopPlaylistMode);

    QAction *randomTrackAction = m_actions[PlaybackModeMenuAction]->menu()->addAction(KIcon("roll"), i18n("Random Track"));
    randomTrackAction->setCheckable(true);
    randomTrackAction->setData(RandomMode);

    QActionGroup *playbackModeActionGroup = new QActionGroup(m_actions[PlaybackModeMenuAction]->menu());
    playbackModeActionGroup->addAction(noRepeatAction);
    playbackModeActionGroup->addAction(currentTrackOnceAction);
    playbackModeActionGroup->addAction(repeatTrackAction);
    playbackModeActionGroup->addAction(repeatPlaylistAction);
    playbackModeActionGroup->addAction(randomTrackAction);

//     m_videoWidget->installEventFilter(this);

    volumeChanged();
    mediaChanged();
    updateSliders();

    connect(m_actions[AspectRatioMenuAction]->menu(), SIGNAL(triggered(QAction*)), this, SLOT(changeAspectRatio(QAction*)));
    connect(m_actions[ChapterMenuAction]->menu(), SIGNAL(triggered(QAction*)), this, SLOT(changeChapter(QAction*)));
    connect(m_actions[AudioMenuAction]->menu(), SIGNAL(triggered(QAction*)), this, SLOT(changeAudioChannel(QAction*)));
    connect(m_actions[SubtitleMenuAction]->menu(), SIGNAL(triggered(QAction*)), this, SLOT(changeSubtitles(QAction*)));
    connect(m_actions[AngleMenuAction]->menu(), SIGNAL(triggered(QAction*)), this, SLOT(changeAngle(QAction*)));
    connect(m_actions[PlayPauseAction], SIGNAL(triggered()), this, SLOT(playPause()));
    connect(m_actions[StopAction], SIGNAL(triggered()), this, SLOT(stop()));
    connect(m_actions[MuteAction], SIGNAL(toggled(bool)), this, SLOT(setAudioMuted(bool)));
    connect(this, SIGNAL(videoAvailableChanged(bool)), m_actions[VideoMenuAction], SLOT(setEnabled(bool)));
    connect(this, SIGNAL(videoAvailableChanged(bool)), m_actions[FullScreenAction], SLOT(setEnabled(bool)));
//     connect(m_mediaObject, SIGNAL(seekableChanged(bool)), this, SIGNAL(seekableChanged(bool)));
//     connect(m_mediaController, SIGNAL(availableChaptersChanged(int)), this, SLOT(availableChaptersChanged()));
//     connect(m_mediaController, SIGNAL(availableAudioChannelsChanged()), this, SLOT(availableAudioChannelsChanged()));
//     connect(m_mediaController, SIGNAL(availableSubtitlesChanged()), this, SLOT(availableSubtitlesChanged()));
//     connect(m_mediaController, SIGNAL(availableAnglesChanged(int)), this, SLOT(availableAnglesChanged()));
//     connect(m_mediaController, SIGNAL(availableTitlesChanged(int)), this, SLOT(availableTitlesChanged()));
    connect(m_brightnessSlider, SIGNAL(valueChanged(int)), this, SLOT(setBrightness(int)));
    connect(m_contrastSlider, SIGNAL(valueChanged(int)), this, SLOT(setContrast(int)));
    connect(m_hueSlider, SIGNAL(valueChanged(int)), this, SLOT(setHue(int)));
    connect(m_saturationSlider, SIGNAL(valueChanged(int)), this, SLOT(setSaturation(int)));
    connect(this, SIGNAL(audioAvailableChanged(bool)), this, SLOT(volumeChanged()));
}

Player::~Player()
{
    if (m_pipeline)
    {
        m_pipeline->setState(QGst::StateNull);
    }
}

void Player::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_hideFullScreenControlsTimer && m_fullScreenWidget && !m_fullScreenUi.controlsWidget->underMouse())
    {
        m_fullScreenUi.videoWidget->setCursor(QCursor(Qt::BlankCursor));
        m_fullScreenUi.titleLabel->hide();
        m_fullScreenUi.controlsWidget->hide();

        m_hideFullScreenControlsTimer = 0;
    }
    else if (event->timerId() == m_playPauseTimer)
    {
        m_actions[PlayPauseAction]->trigger();

        m_playPauseTimer = 0;
    }

    killTimer(event->timerId());
}

void Player::handleBusMessage(const QGst::MessagePtr &message)
{
    switch (message->type())
    {
        case QGst::MessageTag:
            if (m_pipeline)
            {
                const QGst::TagList tags = message.staticCast<QGst::TagMessage>()->taglist();

                if (tags.isEmpty())
                {
                    break;
                }

                QMap<MetaDataKey, QString> metaData;

                if (!tags.artist().isEmpty())
                {
                    metaData[ArtistKey] = tags.artist();
                }

                if (!tags.title().isEmpty())
                {
                    metaData[TitleKey] = tags.title();
                }

                if (!tags.tagValue("ALBUM").toString().isEmpty())
                {
                    metaData[AlbumKey] = tags.tagValue("ALBUM").toString();
                }

                if (tags.trackNumber() > 0)
                {
                    metaData[TrackNumberKey] = QString::number(tags.trackNumber());
                }

                if (!tags.genre().isEmpty())
                {
                    metaData[GenreKey] = tags.genre();
                }

                if (!tags.description().isEmpty())
                {
                    m_metaData[DescriptionKey] = tags.description();
                }

                if (tags.date().isValid())
                {
                    metaData[DateKey] = tags.date().toString();
                }

                if (metaData.isEmpty())
                {
                    break;
                }

                m_metaData = metaData;

                if (isFullScreen())
                {
                    const QString artist = m_metaData[ArtistKey];
                    const QString title = m_metaData[TitleKey];

                    if (artist.isEmpty() || title.isEmpty())
                    {
                        m_fullScreenUi.titleLabel->setText(artist.isEmpty()?title:artist);
                    }
                    else
                    {
                        m_fullScreenUi.titleLabel->setText(QString("%1 - %2").arg(artist).arg(title));
                    }
                }

                if (!MetaDataManager::isAvailable(url()), true)
                {
                    Track track;
                    track.keys = m_metaData;
                    track.duration = duration();

                    MetaDataManager::setMetaData(url(), track);
                }

                Q_EMIT metaDataChanged();
            }

            break;
        case QGst::MessageEos:
            stop();
            trackFinished();

            break;
        case QGst::MessageError:
            if (m_pipeline)
            {
                const QString error = message.staticCast<QGst::ErrorMessage>()->error().message();

                KMessageBox::error(NULL, url().pathOrUrl().replace("%20", " ") + "\n\n" + error);

                Q_EMIT errorOccured(error);
            }

            stop();

            break;
        case QGst::MessageStateChanged:
            if (message->source() == m_pipeline)
            {
                stateChanged(message.staticCast<QGst::StateChangedMessage>()->newState());
            }

            break;
        case QGst::MessageDuration:
            Q_EMIT durationChanged(duration());

            break;
        default:
            break;
    }
}

void Player::stateChanged(QGst::State state)
{
    const PlayerState translatedState = translateState(state);

    mediaChanged();
    updateVideo();

    if (isVideoAvailable() && translatedState == PlayingState && !m_inhibitNotifications)
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

        m_notificationRestrictions->deleteLater();
        m_notificationRestrictions = NULL;
    }

    if (translatedState == StoppedState && isFullScreen())
    {
        m_fullScreenUi.titleLabel->clear();
    }

    Q_EMIT stateChanged(translatedState);
    Q_EMIT audioAvailableChanged(translatedState != StoppedState);
}

void Player::handleVideoChange()
{
    updateVideo();

    Q_EMIT videoAvailableChanged(isVideoAvailable());
}

void Player::volumeChanged(qreal volume)
{
    KIcon icon;

    if (volume >= 0)
    {
        Q_EMIT volumeChanged((int) (volume * 100));
    }

    if (isAudioMuted())
    {
        m_actions[VolumeToggleAction]->setText(i18n("Muted"));

        icon = KIcon("audio-volume-muted");
    }
    else
    {
        m_actions[VolumeToggleAction]->setText(i18n("Volume: %1%", this->volume()));

        if (this->volume() > 65)
        {
            icon = KIcon("audio-volume-high");
        }
        else if (this->volume() < 35)
        {
            icon = KIcon("audio-volume-low");
        }
        else
        {
            icon = KIcon("audio-volume-medium");
        }
    }

    m_actions[MuteAction]->setIcon(icon);
    m_actions[MuteAction]->setEnabled(isAudioAvailable());
    m_actions[VolumeToggleAction]->setIcon(icon);
    m_actions[VolumeToggleAction]->setEnabled(isAudioAvailable());
    m_actions[AudioMenuAction]->setEnabled(isAudioAvailable());

    Q_EMIT modified();
}

void Player::mediaChanged()
{
    const PlayerState state = this->state();
    const bool playingOrPaused = (state != StoppedState);
    const bool hasTracks = ((m_playlist && m_playlist->trackCount())/* || m_mediaObject->currentSource().type() == Phonon::MediaSource::Disc*/);

    m_actions[PlayPauseAction]->setIcon(KIcon((state == PlayingState)?"media-playback-pause":"media-playback-start"));
    m_actions[PlayPauseAction]->setText((state == PlayingState)?i18n("Pause"):i18n("Play"));
    m_actions[PlayPauseAction]->setEnabled(playingOrPaused || hasTracks);
    m_actions[PlayPreviousAction]->setEnabled(hasTracks && m_playlist->trackCount() > 1);
    m_actions[PlayNextAction]->setEnabled(hasTracks && m_playlist->trackCount() > 1);
    m_actions[StopAction]->setEnabled(playingOrPaused);
    m_actions[NavigationMenuAction]->setEnabled(hasTracks);
}

void Player::availableChaptersChanged()
{
//     m_actions[ChapterMenuAction]->menu()->clear();
//     m_actions[ChapterMenuAction]->setEnabled(false);
//
//     m_chaptersGroup->deleteLater();
//     m_chaptersGroup = new QActionGroup(this);
//     m_chaptersGroup->setExclusive(true);
//
//     if (m_mediaController->availableChapters() > 1)
//     {
//         for (int i = 0; i < m_mediaController->availableChapters(); ++i)
//         {
//             QAction *action = m_actions[ChapterMenuAction]->menu()->addAction(i18n("Chapter %1", i));
//             action->setData(i);
//             action->setCheckable(true);
//
//             m_chaptersGroup->addAction(action);
//         }
//
//         m_actions[ChapterMenuAction]->setEnabled(true);
//     }
}

void Player::availableAudioChannelsChanged()
{
//     m_actions[AudioChannelMenuAction]->menu()->clear();
//     m_actions[AudioChannelMenuAction]->setEnabled(false);
//
//     m_audioChannelGroup->deleteLater();
//     m_audioChannelGroup = new QActionGroup(this);
//     m_audioChannelGroup->setExclusive(true);
//
//     if (m_mediaController->availableAudioChannels().count() > 1)
//     {
//         for (int i = 0; i < m_mediaController->availableAudioChannels().count(); ++i)
//         {
//             QAction *action = m_actions[AudioChannelMenuAction]->menu()->addAction(m_mediaController->availableAudioChannels().at(i).name());
//             action->setData(i);
//             action->setCheckable(true);
//
//             m_audioChannelGroup->addAction(action);
//         }
//
//         m_actions[AudioChannelMenuAction]->setEnabled(true);
//     }
}

void Player::availableSubtitlesChanged()
{
//     m_actions[SubtitleMenuAction]->menu()->clear();
//     m_actions[SubtitleMenuAction]->setEnabled(false);
//
//     m_subtitlesGroup->deleteLater();
//     m_subtitlesGroup = new QActionGroup(this);
//     m_subtitlesGroup->setExclusive(true);
//
//     if (m_mediaController->availableSubtitles().count())
//     {
//         for (int i = 0; i < m_mediaController->availableSubtitles().count(); ++i)
//         {
//             QAction *action = m_actions[SubtitleMenuAction]->menu()->addAction(m_mediaController->availableSubtitles().at(i).name());
//             action->setData(i);
//             action->setCheckable(true);
//
//             m_subtitlesGroup->addAction(action);
//         }
//
//         m_actions[SubtitleMenuAction]->setEnabled(true);
//     }
}

void Player::availableAnglesChanged()
{
//     m_actions[AngleMenuAction]->menu()->clear();
//     m_actions[AngleMenuAction]->setEnabled(false);
//
//     m_anglesGroup->deleteLater();
//     m_anglesGroup = new QActionGroup(this);
//     m_anglesGroup->setExclusive(true);
//
//     if (m_mediaController->availableAngles() > 1)
//     {
//         for (int i = 0; i < m_mediaController->availableAngles(); ++i)
//         {
//             QAction *action = m_actions[AngleMenuAction]->menu()->addAction(i18n("Angle %1", i));
//             action->setData(i);
//             action->setCheckable(true);
//
//             m_anglesGroup->addAction(action);
//         }
//
//         m_actions[AngleMenuAction]->setEnabled(true);
//     }
}

void Player::availableTitlesChanged()
{
//     const QString udi = m_mediaObject->currentSource().deviceName();
//     KUrl::List tracks;
//
//     for (int i = 1; i <= m_mediaController->availableTitles(); ++i)
//     {
//         KUrl url = KUrl(QString("disc:/%1/%2").arg(udi).arg(i));
//
//         tracks.append(url);
//
//         MetaDataManager::setMetaData(url, TitleKey, i18n("Track %1", i));
//     }
//
//     Q_EMIT requestDevicePlaylist(udi, tracks);
}

void Player::currentTrackChanged(int track, PlayerReaction reaction)
{
    if (!m_playlist || m_playlist->trackCount() == 0)
    {
//         if (m_mediaObject->currentSource().type() != Phonon::MediaSource::Disc)
        {
            stop();
        }

        return;
    }

    if (reaction == PlayReaction || reaction == PauseReaction)
    {
        setUrl(m_playlist->track(track));

        m_playlist->setLastPlayedDate(QDateTime::currentDateTime());
    }

    switch (reaction)
    {
        case PlayReaction:
            play();

            break;
        case PauseReaction:
            pause();

            break;
        case StopReaction:
            stop();

            break;
        default:
            break;
    }

    Q_EMIT currentTrackChanged();
}

void Player::changeAspectRatio(QAction *action)
{
    setAspectRatio(static_cast<AspectRatio>(action->data().toInt()));
}

void Player::changeChapter(QAction *action)
{
//     m_mediaController->setCurrentChapter(action->data().toInt());
}

void Player::changeAudioChannel(QAction *action)
{
//     m_mediaController->setCurrentAudioChannel(m_mediaController->availableAudioChannels().at(action->data().toInt()));
}

void Player::changeSubtitles(QAction *action)
{
//     m_mediaController->setCurrentSubtitle(m_mediaController->availableSubtitles().at(action->data().toInt()));
}

void Player::changeAngle(QAction *action)
{
//     m_mediaController->setCurrentAngle(action->data().toInt());
}

void Player::trackFinished()
{
    updateVideo();

    if (m_playlist)
    {
        m_playlist->setCurrentTrack(m_playlist->nextTrack(), PlayReaction);
    }

//     m_videoWidget->update();
}

void Player::updateSliders()
{
    disconnect(m_brightnessSlider, SIGNAL(valueChanged(int)), this, SLOT(setBrightness(int)));
    disconnect(m_contrastSlider, SIGNAL(valueChanged(int)), this, SLOT(setContrast(int)));
    disconnect(m_hueSlider, SIGNAL(valueChanged(int)), this, SLOT(setHue(int)));
    disconnect(m_saturationSlider, SIGNAL(valueChanged(int)), this, SLOT(setSaturation(int)));

    m_brightnessSlider->setToolTip(QString(i18n("Brightness: %1%")).arg(brightness()));
    m_contrastSlider->setToolTip(QString(i18n("Contrast: %1%")).arg(contrast()));
    m_hueSlider->setToolTip(QString(i18n("Hue: %1%")).arg(hue()));
    m_saturationSlider->setToolTip(QString(i18n("Saturation: %1%")).arg(saturation()));

    connect(m_brightnessSlider, SIGNAL(valueChanged(int)), this, SLOT(setBrightness(int)));
    connect(m_contrastSlider, SIGNAL(valueChanged(int)), this, SLOT(setContrast(int)));
    connect(m_hueSlider, SIGNAL(valueChanged(int)), this, SLOT(setHue(int)));
    connect(m_saturationSlider, SIGNAL(valueChanged(int)), this, SLOT(setSaturation(int)));
}

void Player::updateVideo()
{
    m_actions[FullScreenAction]->setEnabled(isVideoAvailable());

    if (!isVideoAvailable() && m_fullScreenWidget && m_fullScreenWidget->isFullScreen())
    {
        setFullScreen(false);
    }
    else
    {
        setVideoMode(m_videoMode);
    }
}

void Player::registerAppletVideoWidget(VideoWidget *videoWidget)
{
    m_appletVideoWidget = videoWidget;
    m_appletVideoWidget->installEventFilter(this);
}

void Player::registerDialogVideoWidget(VideoWidget *videoWidget)
{
    m_dialogVideoWidget = videoWidget;
    m_dialogVideoWidget->installEventFilter(this);
}

void Player::seekBackward()
{
    setPosition(position() - (duration() / 30));
}

void Player::seekForward()
{
    setPosition(position() + (duration() / 30));
}

void Player::increaseVolume()
{
    setVolume(qMin(1, (volume() + 10)));
}

void Player::decreaseVolume()
{
    setVolume(qMax(0, (volume() - 10)));
}

void Player::openDisc(const QString &device, PlaylistSource type)
{
//     Phonon::DiscType discType;
//
//     switch (type)
//     {
//         case DvdSource:
//             discType = Phonon::Dvd;
//
//             break;
//         case VcdSource:
//             discType = Phonon::Vcd;
//
//             break;
//         case CdSource:
//             discType = Phonon::Cd;
//
//             break;
//         default:
//             discType = Phonon::NoDisc;
//
//             break;
//     }
//
//     m_mediaObject->setCurrentSource(Phonon::MediaSource(discType, device));
//     m_mediaObject->play();
//
//     Q_EMIT currentTrackChanged();
//
//     if (m_mediaController->availableTitles())
//     {
//         availableTitlesChanged();
//     }
}

void Player::play()
{
    if (!url().isValid() && m_playlist)
    {
        currentTrackChanged(m_playlist->currentTrack(), PlayReaction);
    }
    else if (m_pipeline)
    {
        m_pipeline->setState(QGst::StatePlaying);

        Q_EMIT volumeChanged(volume());
    }
}

void Player::play(int index)
{
    if (m_playlist)
    {
        m_playlist->setCurrentTrack(index, PlayReaction);
    }
}

void Player::playPause()
{
    if (state() == PlayingState)
    {
        pause();
    }
    else
    {
        play();
    }
}

void Player::pause()
{
    if (m_pipeline)
    {
        m_pipeline->setState(QGst::StatePaused);
    }
}

void Player::stop()
{
    if (m_pipeline)
    {
        m_pipeline->setState(QGst::StateNull);

        Q_EMIT videoAvailableChanged(false);
    }

    m_metaData.clear();

    m_url = KUrl();
}

void Player::playPrevious()
{
    if (m_playlist)
    {
        m_playlist->previous(PlayReaction);
    }
}

void Player::playNext()
{
    if (m_playlist)
    {
        m_playlist->next(PlayReaction);
    }
}

void Player::setUrl(const KUrl &url)
{
    if (!m_pipeline)
    {
        m_pipeline = QGst::ElementFactory::make("playbin2").dynamicCast<QGst::Pipeline>();

        if (m_pipeline)
        {
            QGst::BusPtr bus = m_pipeline->bus();
            bus->addSignalWatch();

            QGlib::connect(bus, "message", this, &Player::handleBusMessage);
            QGlib::connect(m_pipeline, "video-changed", this, &Player::handleVideoChange);
        }
    }

    if (m_pipeline)
    {
        stop();

        m_url = url;

        m_pipeline->setProperty("uri", url.url());
    }
}

void Player::setPlaylist(PlaylistModel *playlist)
{
    if (playlist == m_playlist)
    {
        return;
    }

    if (m_playlist)
    {
        disconnect(m_playlist, SIGNAL(playbackModeChanged(PlaybackMode)), this, SIGNAL(playbackModeChanged(PlaybackMode)));
        disconnect(m_playlist, SIGNAL(trackAdded(int)), this, SIGNAL(trackAdded(int)));
        disconnect(m_playlist, SIGNAL(trackRemoved(int)), this, SIGNAL(trackRemoved(int)));
        disconnect(m_playlist, SIGNAL(trackChanged(int)), this, SIGNAL(trackChanged(int)));
        disconnect(m_playlist, SIGNAL(tracksChanged()), this, SIGNAL(playlistChanged()));
        disconnect(m_playlist, SIGNAL(modified()), this, SLOT(mediaChanged()));
        disconnect(m_playlist, SIGNAL(currentTrackChanged(int,PlayerReaction)), this, SLOT(currentTrackChanged(int,PlayerReaction)));
    }

    if (state() != StoppedState)
    {
        stop();
    }

    m_playlist = playlist;

    if (!m_playlist)
    {
        return;
    }

    currentTrackChanged(playlist->currentTrack(), NoReaction);
    mediaChanged();

    Q_EMIT playlistChanged();

    connect(playlist, SIGNAL(playbackModeChanged(PlaybackMode)), this, SIGNAL(playbackModeChanged(PlaybackMode)));
    connect(playlist, SIGNAL(trackAdded(int)), this, SIGNAL(trackAdded(int)));
    connect(playlist, SIGNAL(trackRemoved(int)), this, SIGNAL(trackRemoved(int)));
    connect(playlist, SIGNAL(trackChanged(int)), this, SIGNAL(trackChanged(int)));
    connect(playlist, SIGNAL(tracksChanged()), this, SIGNAL(playlistChanged()));
    connect(playlist, SIGNAL(modified()), this, SLOT(mediaChanged()));
    connect(playlist, SIGNAL(currentTrackChanged(int,PlayerReaction)), this, SLOT(currentTrackChanged(int,PlayerReaction)));
}

void Player::setPosition(qint64 position)
{
    if (isSeekable())
    {
        QGst::SeekEventPtr event = QGst::SeekEvent::create(1.0, QGst::FormatTime, QGst::SeekFlagFlush, QGst::SeekTypeSet, QGst::ClockTime(position * 1000), QGst::SeekTypeNone, QGst::ClockTime::None);

        m_pipeline->sendEvent(event);

        Q_EMIT positionChanged(position);
    }
}

void Player::setVolume(int volume)
{
    if (m_pipeline && volume != this->volume())
    {
        m_pipeline->setProperty("volume", ((double) volume / 10));

        volumeChanged();

        Q_EMIT volumeChanged(volume);
    }
}

void Player::setAudioMuted(bool muted)
{
    if (m_pipeline && muted != isAudioMuted())
    {
        m_pipeline->setProperty("mute", muted);

        volumeChanged();

        Q_EMIT audioMutedChanged(muted);
    }
}

void Player::setPlaybackMode(PlaybackMode mode)
{
    if (m_playlist)
    {
        m_playlist->setPlaybackMode(mode);
    }
}

void Player::setAspectRatio(AspectRatio ratio)
{
    m_aspectRatio = ratio;

//     switch (ratio)
//     {
//         case Ratio4_3:
//             m_videoWidget->setAspectRatio(Phonon::VideoWidget::AspectRatio4_3);
//
//             break;
//         case Ratio16_9:
//             m_videoWidget->setAspectRatio(Phonon::VideoWidget::AspectRatio16_9);
//
//             break;
//         case FitToRatio:
//             m_videoWidget->setAspectRatio(Phonon::VideoWidget::AspectRatioWidget);
//
//             break;
//         default:
//             m_videoWidget->setAspectRatio(Phonon::VideoWidget::AspectRatioAuto);
//
//             ratio = AutomaticRatio;
//
//             break;
//     }

    m_actions[AspectRatioMenuAction]->menu()->actions().at(static_cast<int>(ratio))->setChecked(true);

    Q_EMIT modified();
}

void Player::setVideoMode(bool mode)
{
    m_videoMode = mode;

//     m_videoWidget->setParent(NULL);
//     m_videoWidget->hide();
//
//     if (isFullScreen())
//     {
//         m_appletVideoWidget->setVideoWidget(NULL, false);
//
//         m_dialogVideoWidget->setVideoWidget(NULL, false);
//
//         m_fullScreenUi.videoWidget->layout()->addWidget(m_videoWidget);
//
//         m_videoWidget->show();
//     }
//     else
//     {
//         if (m_fullScreenWidget)
//         {
//             m_fullScreenUi.videoWidget->layout()->removeWidget(m_videoWidget);
//         }
//
//         const bool mode = (state() != StoppedState && isVideoAvailable());
//
//         if (m_videoMode)
//         {
//             m_dialogVideoWidget->setVideoWidget(NULL, false);
//
//             m_appletVideoWidget->setVideoWidget(m_videoWidget, mode);
//         }
//         else
//         {
//             m_appletVideoWidget->setVideoWidget(NULL, false);
//             m_appletVideoWidget->hide();
//
//             m_dialogVideoWidget->setVideoWidget(m_videoWidget, mode);
//         }
//     }
//
//     m_videoWidget->update();
}

void Player::setFullScreen(bool enable)
{
    if (enable)
    {
        if (!isVideoAvailable())
        {
            return;
        }

        if (!m_fullScreenWidget)
        {
            m_fullScreenWidget = new QWidget;
            m_fullScreenWidget->installEventFilter(this);
            m_fullScreenWidget->installEventFilter(parent());

            m_fullScreenUi.setupUi(m_fullScreenWidget);
            m_fullScreenUi.playPauseButton->setDefaultAction(m_actions[PlayPauseAction]);
            m_fullScreenUi.stopButton->setDefaultAction(m_actions[StopAction]);
            m_fullScreenUi.playPreviousButton->setDefaultAction(m_actions[PlayPreviousAction]);
            m_fullScreenUi.playNextButton->setDefaultAction(m_actions[PlayNextAction]);
            m_fullScreenUi.seekSlider->setPlayer(this);
            m_fullScreenUi.muteButton->setDefaultAction(m_actions[MuteAction]);
            m_fullScreenUi.volumeSlider->setPlayer(this);
            m_fullScreenUi.fullScreenButton->setDefaultAction(m_actions[FullScreenAction]);
            m_fullScreenUi.titleLabel->setText(metaData(TitleKey));
            m_fullScreenUi.videoWidget->installEventFilter(this);

            connect(this, SIGNAL(destroyed()), m_fullScreenWidget, SLOT(deleteLater()));
        }

        Q_EMIT fullScreenChanged(true);

        m_fullScreenWidget->showFullScreen();
        m_fullScreenWidget->setWindowTitle(metaData(TitleKey));

        m_fullScreenUi.titleLabel->setText(metaData(TitleKey));
        m_fullScreenUi.titleLabel->hide();
        m_fullScreenUi.controlsWidget->hide();

        m_actions[FullScreenAction]->setIcon(KIcon("view-restore"));
        m_actions[FullScreenAction]->setText(i18n("Exit Full Screen Mode"));

        m_hideFullScreenControlsTimer = startTimer(2000);
    }
    else
    {
        killTimer(m_hideFullScreenControlsTimer);

        Q_EMIT fullScreenChanged(false);

        m_fullScreenWidget->showNormal();
        m_fullScreenWidget->hide();

        m_actions[FullScreenAction]->setIcon(KIcon("view-fullscreen"));
        m_actions[FullScreenAction]->setText(i18n("Full Screen Mode"));

        m_fullScreenUi.videoWidget->setCursor(QCursor(Qt::ArrowCursor));
    }

    setVideoMode(m_videoMode);
}

void Player::setInhibitNotifications(bool inhibit)
{
    m_inhibitNotifications = inhibit;

    if (m_pipeline)
    {
        stateChanged(m_pipeline->currentState());
    }
}

void Player::setBrightness(int value)
{
//     m_videoWidget->setBrightness((value > 0)?(((qreal) value / 50) - 1):-1);

    updateSliders();

    Q_EMIT modified();
}

void Player::setContrast(int value)
{
//     m_videoWidget->setContrast((value > 0)?(((qreal) value / 50) - 1):-1);

    updateSliders();

    Q_EMIT modified();
}

void Player::setHue(int value)
{
//     m_videoWidget->setHue((value > 0)?(((qreal) value / 50) - 1):-1);

    updateSliders();

    Q_EMIT modified();
}

void Player::setSaturation(int value)
{
//     m_videoWidget->setSaturation((value > 0)?(((qreal) value / 50) - 1):-1);

    updateSliders();

    Q_EMIT modified();
}

PlaylistModel* Player::playlist() const
{
    return m_playlist;
}

QAction* Player::action(PlayerAction action) const
{
    return (m_actions.contains(action)?m_actions[action]:NULL);
}

QStringList Player::supportedMimeTypes() const
{
    QStringList mimeTypes;
    mimeTypes << "video/ogg" << "video/x-theora+ogg" << "video/x-ogm+ogg" << "video/x-ms-wmv" << "video/x-msvideo" << "video/x-ms-asf" << "video/x-matroska" << "video/mpeg" << "video/avi" << "video/quicktime" << "video/vnd.rn-realvideo" << "video/x-flic" << "video/mp4" << "video/x-flv" << "video/3gpp" << "application/ogg" << "audio/x-vorbis+ogg" << "audio/mpeg" << "audio/x-flac" << "audio/x-flac+ogg" << "audio/x-musepack" << "audio/x-scpls" << "audio/x-mpegurl" << "application/xspf+xml" << "audio/x-ms-asx";

    return mimeTypes;
}

QString Player::metaData(MetaDataKey key, bool substitute) const
{
    const QString value = m_metaData[key];

    if (value.isEmpty())
    {
        return MetaDataManager::metaData(url(), key, substitute);
    }

    return value;
}

QVariantMap Player::metaData() const
{
    QVariantMap trackData;
    trackData["title"] = metaData(TitleKey, false);
    trackData["artist"] = metaData(ArtistKey, false);
    trackData["album"] = metaData(AlbumKey, false);
    trackData["date"] = metaData(DateKey, false);
    trackData["genre"] = metaData(GenreKey, false);
    trackData["description"] = metaData(DescriptionKey, false);
    trackData["tracknumber"] = metaData(TrackNumberKey, false);
    trackData["time"] = duration();
    trackData["location"] = url().pathOrUrl();

    return trackData;
}

KUrl Player::url() const
{
    return m_url;
}

qint64 Player::duration() const
{
    if (m_pipeline)
    {
        QGst::DurationQueryPtr query = QGst::DurationQuery::create(QGst::FormatTime);

        m_pipeline->query(query);

        return (query->duration() / 1000);
    }

    return 0;
}

qint64 Player::position() const
{
    if (m_pipeline)
    {
        QGst::PositionQueryPtr query = QGst::PositionQuery::create(QGst::FormatTime);

        m_pipeline->query(query);

        return (query->position() / 1000);
    }

    return 0;
}

PlayerState Player::translateState(QGst::State state) const
{
    switch (state)
    {
        case QGst::StatePlaying:
            return PlayingState;
        case QGst::StatePaused:
            return PausedState;
        default:
            return StoppedState;
    }
}

PlaybackMode Player::playbackMode() const
{
    return (m_playlist?m_playlist->playbackMode():SequentialMode);
}

AspectRatio Player::aspectRatio() const
{
    return m_aspectRatio;
}

PlayerState Player::state() const
{
    return (m_pipeline?translateState(m_pipeline->currentState()):StoppedState);
}

int Player::volume() const
{
    return (m_pipeline?(m_pipeline->property("volume").get<double>() * 10):0);
}

int Player::brightness() const
{
    return 0;
//     return ((m_videoWidget->brightness() * 50) + 50);
}

int Player::contrast() const
{
    return 0;
//     return ((m_videoWidget->contrast() * 50) + 50);
}

int Player::hue() const
{
    return 0;
//     return ((m_videoWidget->hue() * 50) + 50);
}

int Player::saturation() const
{
    return 0;
//     return ((m_videoWidget->saturation() * 100) + 50);
}

bool Player::isAudioMuted() const
{
    return (m_pipeline?m_pipeline->property("mute").toBool():false);
}

bool Player::isAudioAvailable() const
{
    return (state() != StoppedState);
}

bool Player::isVideoAvailable() const
{
    return (m_pipeline && m_pipeline->property("n-video").toInt() > 0);
}

bool Player::isSeekable() const
{
    if (m_pipeline)
    {
        QGst::SeekingQueryPtr query = QGst::SeekingQuery::create(QGst::FormatTime);

        m_pipeline->query(query);

        return query->seekable();
    }

    return false;
}

bool Player::isFullScreen() const
{
    return (m_fullScreenWidget && m_fullScreenWidget->isFullScreen());
}

bool Player::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::MouseMove && m_fullScreenWidget && m_fullScreenWidget->isFullScreen())
    {
        killTimer(m_hideFullScreenControlsTimer);

        m_hideFullScreenControlsTimer = startTimer(2000);

        m_fullScreenUi.videoWidget->setCursor(QCursor(Qt::ArrowCursor));
        m_fullScreenUi.titleLabel->show();
        m_fullScreenUi.controlsWidget->show();
    }
    else if (event->type() == QEvent::ContextMenu || event->type() == QEvent::GraphicsSceneContextMenu)
    {
        Q_EMIT requestMenu(QCursor::pos());

        return true;
    }
    else if ((event->type() == QEvent::MouseButtonDblClick && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) || (event->type() == QEvent::GraphicsSceneMouseDoubleClick && static_cast<QGraphicsSceneMouseEvent*>(event)->button() == Qt::LeftButton))
    {
        m_actions[FullScreenAction]->trigger();

        killTimer(m_playPauseTimer);

        m_playPauseTimer = 0;

        return true;
    }
    else if ((event->type() == QEvent::MouseButtonPress && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) || (event->type() == QEvent::GraphicsSceneMousePress && static_cast<QGraphicsSceneMouseEvent*>(event)->button() == Qt::LeftButton))
    {
        if (m_playPauseTimer == 0)
        {
            m_playPauseTimer = startTimer(QApplication::doubleClickInterval() + 10);
        }

        return true;
    }
    else if (event->type() == QEvent::DragEnter)
    {
        static_cast<QDragEnterEvent*>(event)->acceptProposedAction();

        return true;
    }
    else if (event->type() == QEvent::Drop && m_playlist)
    {
        m_playlist->addTracks(KUrl::List::fromMimeData(static_cast<QDropEvent*>(event)->mimeData()), -1, PlayReaction);

        return true;
    }

    return QObject::eventFilter(object, event);
}

}
