/***********************************************************************************
* Copyright (C) 2009 by Marco Martin <notmart@gmail.com>
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

#include "TrackListDBusHandler.h"
#include "TrackListAdaptor.h"
#include "MetaDataManager.h"
#include "PlaylistModel.h"
#include "Player.h"
#include "Constants.h"

#include <QtCore/QList>

#include <KUrl>

TrackListDBusHandler::TrackListDBusHandler(MiniPlayer::Player *parent) : QObject(parent)
{
    m_player = parent;

    setObjectName("TrackListDBusHandler");

    new TrackListAdaptor(this);

    QDBusConnection::sessionBus().registerObject("/TrackList", this);
}

int TrackListDBusHandler::AddTrack(const QString &url, bool playImmediately)
{
    if (KUrl(url).isValid())
    {
        m_player->playlist()->addTracks(KUrl::List(KUrl(url)), -1, playImmediately);

        emit TrackListChange(m_player->playlist()->trackCount());

        return 0;
    }
    else
    {
        return -1;
    }
}

void TrackListDBusHandler::DelTrack(int index)
{
    if (index < m_player->playlist()->trackCount())
    {
        m_player->playlist()->removeTrack(index);

        emit TrackListChange(m_player->playlist()->trackCount());
    }
}

int TrackListDBusHandler::GetCurrentTrack()
{
    return m_player->playlist()->currentTrack();
}

int TrackListDBusHandler::GetLength()
{
    return m_player->playlist()->trackCount();
}

QVariantMap TrackListDBusHandler::GetMetadata(int position)
{
    if (position < 0 || position > (m_player->playlist()->trackCount() - 1))
    {
        return QVariantMap();
    }

    return MiniPlayer::MetaDataManager::metaData(m_player->playlist()->track(position));
}

void TrackListDBusHandler::SetLoop(bool enable)
{
    if (m_player->playlist())
    {
        m_player->playlist()->setPlaybackMode(enable?MiniPlayer::LoopPlaylistMode:MiniPlayer::SequentialMode);
    }
}

void TrackListDBusHandler::SetRandom(bool enable)
{
    if (m_player->playlist())
    {
        m_player->playlist()->setPlaybackMode(enable?MiniPlayer::RandomMode:m_player->playlist()->playbackMode());
    }
}
