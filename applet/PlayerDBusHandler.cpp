/***********************************************************************************
* Copyright (C) 2008 by Marco Martin <notmart@gmail.com>
* Copyright (C) 2009 by Michal Dutkiewicz <emdeck@gmail.com>
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

#include "PlayerDBusHandler.h"
#include "PlayerAdaptor.h"
#include "Applet.h"
#include "MetaDataManager.h"

#include <QtDBus/QDBusConnection>
#include <QtMultimediaKit/QMediaPlayer>
#include <QtMultimediaKit/QMediaPlaylist>

QDBusArgument &operator << (QDBusArgument &argument, const DBusStatus &status)
{
    argument.beginStructure();
    argument << status.Play;
    argument << status.Random;
    argument << status.Repeat;
    argument << status.RepeatPlaylist;
    argument.endStructure();

    return argument;
}

const QDBusArgument &operator >> (const QDBusArgument &argument, DBusStatus &status)
{
    argument.beginStructure();
    argument >> status.Play;
    argument >> status.Random;
    argument >> status.Repeat;
    argument >> status.RepeatPlaylist;
    argument.endStructure();

    return argument;
}

PlayerDBusHandler::PlayerDBusHandler(MiniPlayer::Applet *parent) : QObject(parent)
{
    m_player = parent;

    qDBusRegisterMetaType<DBusStatus>();

    setObjectName("PlayerDBusHandler");

    new PlayerAdaptor(this);

    QDBusConnection::sessionBus().registerObject("/Player", this);

    connect(m_player->mediaPlayer(), SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(stateChanged()));
    connect(m_player->mediaPlayer(), SIGNAL(seekableChanged(bool)), this, SLOT(seekableChanged()));
    connect(m_player->mediaPlayer(), SIGNAL(metaDataChanged()), this, SLOT(trackChanged()));
}

void PlayerDBusHandler::PlayPause()
{
    m_player->playPause();
}

void PlayerDBusHandler::Play()
{
    m_player->play();
}

void PlayerDBusHandler::Pause()
{
    m_player->pause();
}

void PlayerDBusHandler::Stop()
{
    m_player->stop();
}

void PlayerDBusHandler::Next()
{
    m_player->playNext();
}

void PlayerDBusHandler::Prev()
{
    m_player->playPrevious();
}

void PlayerDBusHandler::Repeat(bool enable)
{
    m_player->setPlaybackMode(enable?QMediaPlaylist::CurrentItemInLoop:QMediaPlaylist::Sequential);
}

void PlayerDBusHandler::stateChanged()
{
    emit StatusChange(GetStatus());
    emit CapsChange(GetCaps());
}

void PlayerDBusHandler::seekableChanged()
{
    emit CapsChange(GetCaps());
}

void PlayerDBusHandler::trackChanged()
{
    emit TrackChange(GetMetadata());
}

void PlayerDBusHandler::VolumeSet(int value)
{
    m_player->mediaPlayer()->setVolume(value);
}

void PlayerDBusHandler::PositionSet(int position)
{
    m_player->mediaPlayer()->setPosition(position);
}

DBusStatus PlayerDBusHandler::GetStatus()
{
    DBusStatus status;

    switch (m_player->mediaPlayer()->state())
    {
        case QMediaPlayer::PlayingState:
            status.Play = 0;
        break;
        case QMediaPlayer::PausedState:
            status.Play = 1;
        break;
        case QMediaPlayer::StoppedState:
        default:
            status.Play = 2;
    }

    status.Random = ((m_player->playbackMode() == QMediaPlaylist::Random)?1:0);
    status.Repeat = ((m_player->playbackMode() == QMediaPlaylist::CurrentItemInLoop)?1:0);
    status.RepeatPlaylist = ((m_player->playbackMode() == QMediaPlaylist::Loop)?1:0);

    return status;
}

QVariantMap PlayerDBusHandler::GetMetadata()
{
    return m_player->metaDataManager()->metaData(m_player->mediaPlayer());
}

int PlayerDBusHandler::GetCaps()
{
    int caps = NONE;

    caps |= CAN_GO_NEXT;

    caps |= CAN_GO_PREV;

    if (m_player->mediaPlayer()->state() == QMediaPlayer::PlayingState)
    {
        caps |= CAN_PAUSE;
    }

    if (m_player->mediaPlayer()->state() == QMediaPlayer::PausedState)
    {
        caps |= CAN_PLAY;
    }

    if (m_player->mediaPlayer()->isSeekable())
    {
        caps |= CAN_SEEK;
    }

    caps |= CAN_PROVIDE_METADATA;

    caps |= CAN_HAS_TRACKLIST;

    return caps;
}

int PlayerDBusHandler::VolumeGet() const
{
     return m_player->mediaPlayer()->volume();
}

qint64 PlayerDBusHandler::PositionGet() const
{
    return m_player->mediaPlayer()->position();
}
