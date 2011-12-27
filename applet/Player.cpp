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

#include "Player.h"
#include "MetaDataManager.h"
#include "PlaylistModel.h"
#include "VideoWidget.h"

#include <QtCore/QFileInfo>
#include <QtGui/QLayout>
#include <QtGui/QActionGroup>
#include <QtGui/QGraphicsSceneMouseEvent>

#include <KIcon>
#include <KMenu>
#include <KLocale>
#include <KMessageBox>

namespace MiniPlayer
{

Player::Player(QObject *parent) : QObject(parent),
    m_mediaObject(new Phonon::MediaObject(this)),
    m_mediaController(new Phonon::MediaController(m_mediaObject)),
    m_audioOutput(new Phonon::AudioOutput(this)),
    m_videoWidget(new Phonon::VideoWidget()),
    m_metaDataManager(new MetaDataManager(this)),
    m_playlist(NULL),
    m_appletVideoWidget(NULL),
    m_dialogVideoWidget(NULL),
    m_fullScreenVideoWidget(NULL),
    m_playbackMode(SequentialMode),
    m_aspectRatio(AutomaticRatio),
    m_videoMode(false),
    m_fullScreenMode(false)
{
    m_videoWidget->setScaleMode(Phonon::VideoWidget::FitInView);
    m_videoWidget->setWindowIcon(KIcon("applications-multimedia"));
    m_videoWidget->setAcceptDrops(true);

    Phonon::createPath(m_mediaObject, m_audioOutput);
    Phonon::createPath(m_mediaObject, m_videoWidget);

    m_actions[OpenMenuAction] = new QAction(i18n("Open"), this);
    m_actions[OpenMenuAction]->setMenu(new KMenu());
    m_actions[OpenFileAction] = m_actions[OpenMenuAction]->menu()->addAction(KIcon("document-open"), i18n("Open File"));
    m_actions[OpenFileAction]->setShortcut(QKeySequence(Qt::Key_O));
    m_actions[OpenUrlAction] = m_actions[OpenMenuAction]->menu()->addAction(KIcon("uri-mms"), i18n("Open URL"));
    m_actions[OpenUrlAction]->setShortcut(QKeySequence(Qt::Key_U));

    m_actions[PlayPauseAction] = new QAction(KIcon("media-playback-start"), i18n("Play"), this);
    m_actions[PlayPauseAction]->setEnabled(false);
    m_actions[PlayPauseAction]->setShortcut(QKeySequence(Qt::Key_Space));
    m_actions[StopAction] = new QAction(KIcon("media-playback-stop"), i18n("Stop"), this);
    m_actions[StopAction]->setEnabled(false);
    m_actions[StopAction]->setShortcut(QKeySequence(Qt::Key_S));

    m_actions[NavigationMenuAction] = new QAction(i18n("Navigation"), this);
    m_actions[NavigationMenuAction]->setMenu(new KMenu());
    m_actions[NavigationMenuAction]->setEnabled(false);
    m_actions[PlayNextAction] = m_actions[NavigationMenuAction]->menu()->addAction(KIcon("media-skip-backward"), i18n("Previous"), this, SLOT(playPrevious()), QKeySequence(Qt::Key_PageDown));
    m_actions[PlayPreviousAction] = m_actions[NavigationMenuAction]->menu()->addAction(KIcon("media-skip-forward"), i18n("Next"), this, SLOT(playNext()), QKeySequence(Qt::Key_PageUp));
    m_actions[NavigationMenuAction]->menu()->addSeparator();
    m_actions[SeekBackwardAction] = m_actions[NavigationMenuAction]->menu()->addAction(KIcon("media-seek-backward"), i18n("Seek Backward"), this, SLOT(seekBackward()), QKeySequence(Qt::Key_Left));
    m_actions[SeekForwardAction] = m_actions[NavigationMenuAction]->menu()->addAction(KIcon("media-seek-forward"), i18n("Seek Forward"), this, SLOT(seekForward()), QKeySequence(Qt::Key_Right));
    m_actions[NavigationMenuAction]->menu()->addSeparator();
    m_actions[SeekToAction] = m_actions[NavigationMenuAction]->menu()->addAction(KIcon("go-jump"), i18n("Jump to Position"));
    m_actions[SeekToAction]->setShortcut(QKeySequence(Qt::Key_G));

    m_actions[VolumeAction] = new QAction(KIcon("player-volume"), i18n("Volume"), this);
    m_actions[VolumeAction]->setShortcut(QKeySequence(Qt::Key_V));
    m_actions[AudioMenuAction] = new QAction(i18n("Audio"), this);
    m_actions[AudioMenuAction]->setMenu(new KMenu());
    m_actions[IncreaseVolumeAction] = m_actions[AudioMenuAction]->menu()->addAction(KIcon("audio-volume-high"), i18n("Increase Volume"), this, SLOT(increaseVolume()), QKeySequence(Qt::Key_Plus));
    m_actions[DecreaseVolumeAction] = m_actions[AudioMenuAction]->menu()->addAction(KIcon("audio-volume-low"), i18n("Decrease Volume"), this, SLOT(decreaseVolume()), QKeySequence(Qt::Key_Minus));
    m_actions[AudioMenuAction]->menu()->addSeparator();
    m_actions[MuteAction] = m_actions[AudioMenuAction]->menu()->addAction(KIcon("audio-volume-medium"), i18n("Mute Volume"));
    m_actions[MuteAction]->setCheckable(true);
    m_actions[MuteAction]->setShortcut(QKeySequence(Qt::Key_M));

    m_actions[VideoMenuAction] = new QAction(i18n("Video"), this);
    m_actions[VideoMenuAction]->setMenu(new KMenu());
    m_actions[VideoMenuAction]->setEnabled(false);
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

    m_actions[VideoMenuAction]->menu()->addSeparator();
    m_actions[FullScreenAction] = m_actions[VideoMenuAction]->menu()->addAction(KIcon("view-fullscreen"), i18n("Full Screen Mode"));
    m_actions[FullScreenAction]->setEnabled(false);
    m_actions[FullScreenAction]->setShortcut(QKeySequence(Qt::Key_F));

    m_actions[PlaybackModeMenuAction] = new QAction(KIcon("edit-redo"), i18n("Playback Mode"), this);
    m_actions[PlaybackModeMenuAction]->setMenu(new KMenu());

    QAction *noRepeatAction = m_actions[PlaybackModeMenuAction]->menu()->addAction(KIcon("dialog-cancel"), i18n("No Repeat"));
    noRepeatAction->setCheckable(true);
    noRepeatAction->setData(SequentialMode);

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
    playbackModeActionGroup->addAction(repeatTrackAction);
    playbackModeActionGroup->addAction(repeatPlaylistAction);
    playbackModeActionGroup->addAction(randomTrackAction);

    volumeChanged();
    mediaChanged();

    connect(m_actions[VideoMenuAction]->menu(), SIGNAL(triggered(QAction*)), this, SLOT(changeAspectRatio(QAction*)));
    connect(m_actions[PlaybackModeMenuAction]->menu(), SIGNAL(triggered(QAction*)), this, SLOT(changePlaybackMode(QAction*)));
    connect(m_actions[PlayPauseAction], SIGNAL(triggered()), this, SLOT(playPause()));
    connect(m_actions[StopAction], SIGNAL(triggered()), this, SLOT(stop()));
    connect(m_actions[MuteAction], SIGNAL(toggled(bool)), this, SLOT(setAudioMuted(bool)));
    connect(m_mediaObject, SIGNAL(finished()), this, SLOT(trackFinished()));
    connect(m_mediaObject, SIGNAL(stateChanged(Phonon::State,Phonon::State)), this, SLOT(stateChanged(Phonon::State)));
    connect(m_mediaObject, SIGNAL(metaDataChanged()), this, SIGNAL(metaDataChanged()));
    connect(m_mediaObject, SIGNAL(totalTimeChanged(qint64)), this, SIGNAL(durationChanged(qint64)));
    connect(m_mediaObject, SIGNAL(hasVideoChanged(bool)), this, SIGNAL(videoAvailableChanged(bool)));
    connect(m_mediaObject, SIGNAL(hasVideoChanged(bool)), this, SLOT(videoChanged()));
    connect(m_mediaObject, SIGNAL(hasVideoChanged(bool)), m_actions[VideoMenuAction], SLOT(setEnabled(bool)));
    connect(m_mediaObject, SIGNAL(hasVideoChanged(bool)), m_actions[FullScreenAction], SLOT(setEnabled(bool)));
    connect(m_mediaObject, SIGNAL(seekableChanged(bool)), this, SIGNAL(seekableChanged(bool)));
    connect(m_audioOutput, SIGNAL(volumeChanged(qreal)), this, SLOT(volumeChanged(qreal)));
    connect(m_audioOutput, SIGNAL(volumeChanged(qreal)), this, SLOT(volumeChanged()));
    connect(m_audioOutput, SIGNAL(mutedChanged(bool)), this, SIGNAL(audioMutedChanged(bool)));
    connect(m_audioOutput, SIGNAL(mutedChanged(bool)), this, SLOT(volumeChanged()));
    connect(this, SIGNAL(destroyed()), m_videoWidget, SLOT(deleteLater()));
}

void Player::volumeChanged(qreal volume)
{
    KIcon icon;

    if (volume >= 0)
    {
        emit volumeChanged((int) (volume * 100));
    }

    if (isAudioMuted())
    {
        m_actions[VolumeAction]->setText(i18n("Muted"));

        icon = KIcon("audio-volume-muted");
    }
    else
    {
        m_actions[VolumeAction]->setText(i18n("Volume: %1%", this->volume()));

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
    m_actions[VolumeAction]->setIcon(icon);
    m_actions[VolumeAction]->setEnabled(isAudioAvailable());
    m_actions[AudioMenuAction]->setEnabled(isAudioAvailable());

    emit configNeedsSaving();
}

void Player::videoChanged()
{
    if (isVideoAvailable())
    {
        setVideoMode(m_videoMode);
    }
    else
    {
        m_appletVideoWidget->setVideoWidget(NULL);

        if (m_dialogVideoWidget)
        {
            m_dialogVideoWidget->setVideoWidget(NULL);
        }
    }
}

void Player::mediaChanged()
{
    const PlayerState state = this->state();
    const bool playingOrPaused = (state == PlayingState || state == PausedState);
    const bool hasTracks = (m_playlist && m_playlist->trackCount());

    m_actions[PlayPauseAction]->setIcon(KIcon((state == PlayingState)?"media-playback-pause":"media-playback-start"));
    m_actions[PlayPauseAction]->setText((state == PlayingState)?i18n("Pause"):i18n("Play"));
    m_actions[PlayPauseAction]->setEnabled(playingOrPaused || hasTracks);
    m_actions[StopAction]->setEnabled(playingOrPaused);
    m_actions[NavigationMenuAction]->setEnabled(hasTracks);
}

void Player::currentTrackChanged(int track, bool play)
{
    if (m_playlist)
    {
        m_mediaObject->setCurrentSource(Phonon::MediaSource(m_playlist->track(track)));

        if (play)
        {
            this->play();
        }
    }
}

void Player::stateChanged(Phonon::State state)
{
    mediaChanged();

    if (this->state() == StoppedState)
    {
        videoChanged();
    }

    if (state == Phonon::ErrorState && m_mediaObject->errorType() != Phonon::NoError)
    {
        KMessageBox::error(NULL, m_mediaObject->currentSource().url().toString().replace("%20", " ") + "\n\n" + m_mediaObject->errorString());

        emit errorOccured(m_mediaObject->errorString());
    }

    emit translateState(state);
}

void Player::changeAspectRatio(QAction *action)
{
    setAspectRatio(static_cast<AspectRatio>(action->data().toInt()));
}

void Player::changePlaybackMode(QAction *action)
{
    setPlaybackMode(static_cast<PlaybackMode>(action->data().toInt()));
}

void Player::trackFinished()
{
    videoChanged();

    if (m_playlist)
    {
        m_playlist->setCurrentTrack(m_playlist->nextTrack(), true);
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

    setVideoMode(m_videoMode);
}

void Player::registerFullScreenVideoWidget(QWidget *videoWidget)
{
    m_fullScreenVideoWidget = videoWidget;
    m_fullScreenVideoWidget->installEventFilter(this);
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

void Player::play()
{
    m_mediaObject->play();
}

void Player::play(int index)
{
    if (m_playlist)
    {
        m_playlist->setCurrentTrack(index);

        m_mediaObject->play();
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
    m_mediaObject->pause();
}

void Player::stop()
{
    m_mediaObject->stop();
}

void Player::playPrevious()
{
    if (m_playlist)
    {
        m_playlist->previous();

        play();
    }
}

void Player::playNext()
{
    if (m_playlist)
    {
        m_playlist->next();

        play();
    }
}

void Player::setPlaylist(PlaylistModel *playlist)
{
    if (m_playlist)
    {
        disconnect(m_playlist, SIGNAL(needsSaving()), this, SLOT(mediaChanged()));
        disconnect(m_playlist, SIGNAL(currentTrackChanged(int,bool)), this, SLOT(currentTrackChanged(int,bool)));
    }

    m_playlist = playlist;

    currentTrackChanged(playlist->currentTrack(), (state() == PlayingState));
    setPlaybackMode(m_playbackMode);
    mediaChanged();

    connect(playlist, SIGNAL(needsSaving()), this, SLOT(mediaChanged()));
    connect(playlist, SIGNAL(currentTrackChanged(int,bool)), this, SLOT(currentTrackChanged(int,bool)));
}

void Player::setPosition(qint64 position)
{
    m_mediaObject->seek(position);
}

void Player::setVolume(int volume)
{
    m_audioOutput->setVolume((qreal) volume / 100);
}

void Player::setAudioMuted(bool muted)
{
    m_audioOutput->setMuted(muted);
}

void Player::setPlaybackMode(PlaybackMode mode)
{
    m_playbackMode = mode;

    m_actions[PlaybackModeMenuAction]->menu()->actions().at(static_cast<int>(mode))->setChecked(true);

    if (m_playlist)
    {
        m_playlist->setPlaybackMode(mode);
    }

    emit configNeedsSaving();
}

void Player::setAspectRatio(AspectRatio ratio)
{
    m_aspectRatio = ratio;

    switch (ratio)
    {
        case Ratio4_3:
            m_videoWidget->setAspectRatio(Phonon::VideoWidget::AspectRatio4_3);
        break;
        case Ratio16_9:
            m_videoWidget->setAspectRatio(Phonon::VideoWidget::AspectRatio16_9);
        break;
        case FitToRatio:
            m_videoWidget->setAspectRatio(Phonon::VideoWidget::AspectRatioWidget);
        break;
        default:
            m_videoWidget->setAspectRatio(Phonon::VideoWidget::AspectRatioAuto);

            ratio = AutomaticRatio;
        break;
    }

    m_actions[AspectRatioMenuAction]->menu()->actions().at(static_cast<int>(ratio))->setChecked(true);

    emit configNeedsSaving();
}

void Player::setVideoMode(bool mode)
{
    m_videoMode = mode;

    if (m_fullScreenMode && m_fullScreenVideoWidget)
    {
        m_appletVideoWidget->setVideoWidget(NULL);

        if (m_dialogVideoWidget)
        {
            m_dialogVideoWidget->setVideoWidget(NULL);
        }

        m_fullScreenVideoWidget->layout()->addWidget(m_videoWidget);
    }
    else
    {
        if (m_fullScreenVideoWidget)
        {
            m_fullScreenVideoWidget->layout()->removeWidget(m_videoWidget);
        }

        if (m_videoMode)
        {
            if (m_dialogVideoWidget)
            {
                m_dialogVideoWidget->setVideoWidget(NULL);
            }

            m_appletVideoWidget->setVideoWidget(m_videoWidget);
        }
        else if (m_dialogVideoWidget)
        {
            m_appletVideoWidget->setVideoWidget(NULL);

            m_dialogVideoWidget->setVideoWidget(m_videoWidget);
        }
    }

    if (!isVideoAvailable())
    {
        videoChanged();
    }
}

void Player::setFullScreen(bool enable)
{
    m_fullScreenMode = enable;

    setVideoMode(m_videoMode);
}

QString Player::errorString() const
{
    return m_mediaObject->errorString();
}

QString Player::title() const
{
    const QStringList titles = m_mediaObject->metaData(Phonon::TitleMetaData);

    if (titles.isEmpty() || titles.first().isEmpty())
    {
        return QFileInfo(m_mediaObject->currentSource().url().toString()).completeBaseName().replace("%20", " ");
    }
    else
    {
        return titles.first();
    }
}

MetaDataManager* Player::metaDataManager()
{
    return m_metaDataManager;
}

PlaylistModel* Player::playlist() const
{
    return m_playlist;
}

QAction* Player::action(PlayerAction action) const
{
    return (m_actions.contains(action)?m_actions[action]:NULL);
}

KUrl Player::url() const
{
    return KUrl(m_mediaObject->currentSource().url());
}

qint64 Player::duration() const
{
    return m_mediaObject->totalTime();
}

qint64 Player::position() const
{
    return m_mediaObject->currentTime();
}

PlayerState Player::translateState(Phonon::State state) const
{
    switch (state)
    {
        case Phonon::PlayingState:
            return PlayingState;
        break;
        case Phonon::PausedState:
            return PausedState;
        break;
        default:
            return StoppedState;
        break;
    }
}

PlaybackMode Player::playbackMode() const
{
    return m_playbackMode;
}

AspectRatio Player::aspectRatio() const
{
    return m_aspectRatio;
}

PlayerState Player::state() const
{
    return translateState(m_mediaObject->state());
}

int Player::volume() const
{
    return (m_audioOutput->volume() * 100);
}

bool Player::isAudioMuted() const
{
    return m_audioOutput->isMuted();
}

bool Player::isAudioAvailable() const
{
    return (state() != StoppedState);
}

bool Player::isVideoAvailable() const
{
    return m_mediaObject->hasVideo();
}

bool Player::isSeekable() const
{
    return m_mediaObject->isSeekable();
}

bool Player::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::ContextMenu || event->type() == QEvent::GraphicsSceneContextMenu)
    {
        emit requestMenu(QCursor::pos());

        return true;
    }
    else if ((event->type() == QEvent::MouseButtonDblClick && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) || (event->type() == QEvent::GraphicsSceneMouseDoubleClick && static_cast<QGraphicsSceneMouseEvent*>(event)->button() == Qt::LeftButton))
    {
        m_actions[FullScreenAction]->trigger();

        return true;
    }
    else if (event->type() == QEvent::DragEnter)
    {
        static_cast<QDragEnterEvent*>(event)->acceptProposedAction();

        return true;
    }
    else if (event->type() == QEvent::Drop && m_playlist)
    {
        m_playlist->addTracks(KUrl::List::fromMimeData(static_cast<QDropEvent*>(event)->mimeData()), -1, true);

        return true;
    }

    return QObject::eventFilter(object, event);
}

}
