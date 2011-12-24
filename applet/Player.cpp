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

#include <KIcon>
#include <KMenu>
#include <KLocale>

namespace MiniPlayer
{

Player::Player(QObject *parent) : QObject(parent),
    m_mediaPlayer(new QMediaPlayer(this))
{
    m_actions[OpenMenuAction] = new QAction(i18n("Open"), this);
    m_actions[OpenMenuAction]->setMenu(new KMenu());
    m_actions[OpenFileAction] = m_actions[OpenMenuAction]->menu()->addAction(KIcon("document-open"), i18n("Open File"), this, SLOT(openFiles()), QKeySequence(Qt::Key_O));
    m_actions[OpenUrlAction] = m_actions[OpenMenuAction]->menu()->addAction(KIcon("uri-mms"), i18n("Open URL"), this, SLOT(openUrl()), QKeySequence(Qt::Key_U));
    m_actions[PlayPauseAction] = new QAction(KIcon("media-playback-start"), i18n("Play"), this);
    m_actions[PlayPauseAction]->setEnabled(false);
    m_actions[PlayPauseAction]->setShortcut(QKeySequence(Qt::Key_Space));
    m_actions[StopAction] = new QAction(KIcon("media-playback-stop"), i18n("Stop"), this);
    m_actions[StopAction]->setEnabled(false);
    m_actions[StopAction]->setShortcut(QKeySequence(Qt::Key_S));
    m_actions[VolumeAction] = new QAction(KIcon("player-volume"), i18n("Volume"), this);
    m_actions[VolumeAction]->setShortcut(QKeySequence(Qt::Key_V));
    m_actions[AudioMenuAction] = new QAction(i18n("Audio"), this);
    m_actions[AudioMenuAction]->setMenu(new KMenu());
    m_actions[IncreaseVolumeAction] = m_actions[AudioMenuAction]->menu()->addAction(KIcon("audio-volume-high"), i18n("Increase Volume"), this, SLOT(increaseVolume()), QKeySequence(Qt::Key_Plus));
    m_actions[DecreaseVolumeAction] = m_actions[AudioMenuAction]->menu()->addAction(KIcon("audio-volume-low"), i18n("Decrease Volume"), this, SLOT(decreaseVolume()), QKeySequence(Qt::Key_Minus));
    m_actions[AudioMenuAction]->menu()->addSeparator();
    m_actions[MuteAction]->menu()->addAction(KIcon("audio-volume-medium"), i18n("Mute Volume"));
    m_actions[MuteAction]->setCheckable(true);
    m_actions[MuteAction]->setShortcut(QKeySequence(Qt::Key_M));
    m_actions[NavigationMenuAction] = new QAction(i18n("Navigation"), this);
    m_actions[NavigationMenuAction]->setMenu(new KMenu());
    m_actions[NavigationMenuAction]->setEnabled(false);
    m_actions[PlayNextAction] = m_actions[NavigationMenuAction]->menu()->addAction(KIcon("media-skip-backward"), i18n("Previous"), this, SLOT(playPrevious()), QKeySequence(Qt::Key_PageDown));
    m_actions[PlayPreviousAction] = m_actions[NavigationMenuAction]->menu()->addAction(KIcon("media-skip-forward"), i18n("Next"), this, SLOT(playNext()), QKeySequence(Qt::Key_PageUp));
    m_actions[NavigationMenuAction]->menu()->addSeparator();
    m_actions[SeekBackwardAction] = m_actions[NavigationMenuAction]->menu()->addAction(KIcon("media-seek-backward"), i18n("Seek Backward"), this, SLOT(seekBackward()), QKeySequence(Qt::Key_Left));
    m_actions[SeekForwardAction] = m_actions[NavigationMenuAction]->menu()->addAction(KIcon("media-seek-forward"), i18n("Seek Forward"), this, SLOT(seekForward()), QKeySequence(Qt::Key_Right));
    m_actions[NavigationMenuAction]->menu()->addSeparator();
    m_actions[SeekToAction] = m_actions[NavigationMenuAction]->menu()->addAction(KIcon("go-jump"), i18n("Jump to Position"), this, SLOT(toggleJumpToPosition()), QKeySequence(Qt::Key_G));



    connect(m_actions[MuteAction], SIGNAL(toggled(bool)), this, SLOT(setMuted(bool)));
    connect(m_mediaPlayer, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(stateChanged(QMediaPlayer::State)));
    connect(m_mediaPlayer, SIGNAL(error(QMediaPlayer::Error)), this, SLOT(errorOccured(QMediaPlayer::Error)));
    connect(m_mediaPlayer, SIGNAL(metaDataChanged()), this, SIGNAL(metaDataChanged()));
    connect(m_mediaPlayer, SIGNAL(durationChanged(qint64)), this, SIGNAL(durationChanged(qint64)));
    connect(m_mediaPlayer, SIGNAL(volumeChanged(int)), this, SIGNAL(volumeChanged(int)));
    connect(m_mediaPlayer, SIGNAL(mutedChanged(bool)), this, SIGNAL(audioMutedChanged(bool)));
    connect(m_mediaPlayer, SIGNAL(audioAvailableChanged(bool)), this, SIGNAL(audioAvailableChanged(bool)));
    connect(m_mediaPlayer, SIGNAL(videoAvailableChanged(bool)), this, SIGNAL(videoAvailableChanged(bool)));
    connect(m_mediaPlayer, SIGNAL(seekableChanged(bool)), this, SIGNAL(seekableChanged(bool)));
}

void Player::stateChanged(QMediaPlayer::State state)
{
    emit translateState(state);
}

void Player::errorOccured(QMediaPlayer::Error error)
{
    if (error != QMediaPlayer::NoError)
    {
        emit errorOccured(m_mediaPlayer->errorString());
    }
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
    m_mediaPlayer->play();
}

void Player::playPause()
{
    if (state() == PlayingState)
    {
        play();
    }
    else
    {
        pause();
    }
}

void Player::pause()
{
    m_mediaPlayer->pause();
}

void Player::stop()
{
    m_mediaPlayer->stop();
}

void Player::setPlaylist(QMediaPlaylist *playlist)
{
    m_mediaPlayer->setPlaylist(playlist);
}

void Player::setPosition(qint64 position)
{
    m_mediaPlayer->setPosition(position);
}

void Player::setVolume(int volume)
{
    m_mediaPlayer->setVolume(volume);
}

void Player::setAudioMuted(bool muted)
{
    m_mediaPlayer->setMuted(muted);
}

QString Player::errorString() const
{
    return m_mediaPlayer->errorString();
}

QMediaPlaylist* Player::playlist() const
{
    return m_mediaPlayer->playlist();
}

QAction* Player::action(PlayerAction action) const
{
    return (m_actions.contains(action)?m_actions[action]:NULL);
}

qint64 Player::duration() const
{
    return m_mediaPlayer->duration();
}

qint64 Player::position() const
{
    return m_mediaPlayer->position();
}

PlayerState Player::translateState(QMediaPlayer::State state) const
{
    switch (state)
    {
        case QMediaPlayer::PlayingState:
            return PlayingState;
        break;
        case QMediaPlayer::PausedState:
            return PausedState;
        break;
        default:
            return StoppedState;
        break;
    }
}

PlayerState Player::state() const
{
    return translateState(m_mediaPlayer->state());
}

int Player::volume() const
{
    return m_mediaPlayer->volume();
}

bool Player::isAudioMuted() const
{
    return m_mediaPlayer->isMuted();
}

bool Player::isAudioAvailable() const
{
    return m_mediaPlayer->isAudioAvailable();
}

bool Player::isVideoAvailable() const
{
    return m_mediaPlayer->isVideoAvailable();
}

bool Player::isSeekable() const
{
    return m_mediaPlayer->isSeekable();
}

}
