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
#include "PlaylistModel.h"
#include "MetaDataManager.h"
#include "Player.h"
#include "Constants.h"

#include <QtDBus/QDBusConnection>

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

PlayerDBusHandler::PlayerDBusHandler(MiniPlayer::Player *parent) : QObject(parent)
{
    m_player = parent;

    qDBusRegisterMetaType<DBusStatus>();

    setObjectName("PlayerDBusHandler");

    new PlayerAdaptor(this);

    QDBusConnection::sessionBus().registerObject("/Player", this);

    connect(m_player, SIGNAL(stateChanged(PlayerState)), this, SLOT(stateChanged()));
    connect(m_player, SIGNAL(seekableChanged(bool)), this, SLOT(seekableChanged()));
    connect(m_player, SIGNAL(metaDataChanged()), this, SLOT(trackChanged()));
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
    if (m_player->playlist())
    {
        m_player->playlist()->setPlaybackMode(enable?MiniPlayer::LoopTrackMode:MiniPlayer::SequentialMode);
    }
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
    m_player->setVolume(value);
}

void PlayerDBusHandler::PositionSet(int position)
{
    m_player->setPosition(position);
}

DBusStatus PlayerDBusHandler::GetStatus()
{
    DBusStatus status;

    switch (m_player->state())
    {
        case MiniPlayer::PlayingState:
            status.Play = 0;
        break;
        case MiniPlayer::PausedState:
            status.Play = 1;
        break;
        default:
            status.Play = 2;
    }

    status.Random = ((m_player->playlist() && m_player->playlist()->playbackMode() == MiniPlayer::RandomMode)?1:0);
    status.Repeat = ((m_player->playlist() && m_player->playlist()->playbackMode() == MiniPlayer::LoopTrackMode)?1:0);
    status.RepeatPlaylist = ((m_player->playlist() && m_player->playlist()->playbackMode() == MiniPlayer::LoopPlaylistMode)?1:0);

    return status;
}

QVariantMap PlayerDBusHandler::GetMetadata()
{
    return MiniPlayer::MetaDataManager::metaData(m_player->url());
}

int PlayerDBusHandler::GetCaps()
{
    int caps = NONE;

    caps |= CAN_GO_NEXT;
    caps |= CAN_GO_PREV;
    caps |= CAN_PROVIDE_METADATA;
    caps |= CAN_HAS_TRACKLIST;

    if (m_player->state() == MiniPlayer::PlayingState)
    {
        caps |= CAN_PAUSE;
    }

    if (m_player->state() == MiniPlayer::PausedState)
    {
        caps |= CAN_PLAY;
    }

    if (m_player->isSeekable())
    {
        caps |= CAN_SEEK;
    }

    return caps;
}

int PlayerDBusHandler::VolumeGet() const
{
     return m_player->volume();
}

qint64 PlayerDBusHandler::PositionGet() const
{
    return m_player->position();
}
