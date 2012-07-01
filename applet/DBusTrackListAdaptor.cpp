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

#include "DBusTrackListAdaptor.h"
#include "Player.h"
#include "PlaylistModel.h"
#include "MetaDataManager.h"

namespace MiniPlayer
{

DBusTrackListAdaptor::DBusTrackListAdaptor(QObject *parent, Player *player) : QDBusAbstractAdaptor(parent),
    m_player(player)
{
    connect(m_player, SIGNAL(playlistChanged()), this, SLOT(emitTrackListReplaced()));
    connect(m_player, SIGNAL(trackAdded(int)), this, SLOT(emitTrackAdded(int)));
    connect(m_player, SIGNAL(trackRemoved(int)), this, SLOT(emitTrackRemoved(int)));
    connect(m_player, SIGNAL(trackChanged(int)), this, SLOT(emitTrackMetadataChanged(int)));
}

void DBusTrackListAdaptor::AddTrack(const QString &uri, const QDBusObjectPath &afterTrack, bool setAsCurrent) const
{
    if (m_player->playlist())
    {
        m_player->playlist()->addTracks(KUrl::List(uri), (trackNumber(afterTrack.path()) + 1), (setAsCurrent?PlayReaction:NoReaction));
    }
}

void DBusTrackListAdaptor::RemoveTrack(const QDBusObjectPath &trackId) const
{
    if (m_player->playlist())
    {
        m_player->playlist()->removeTrack(trackNumber(trackId.path()));
    }
}

void DBusTrackListAdaptor::GoTo(const QDBusObjectPath &trackId) const
{
    if (m_player->playlist())
    {
        m_player->playlist()->setCurrentTrack(trackNumber(trackId.path()), PlayReaction);
    }
}

void DBusTrackListAdaptor::emitTrackListReplaced()
{
    const int track = (m_player->playlist()?m_player->playlist()->currentTrack():0);

    Q_EMIT TrackListReplaced(Tracks(), QDBusObjectPath(((track >= 0)?QString("/track_%1").arg(track):"/org/mpris/MediaPlayer2/TrackList/NoTrack")));
}

void DBusTrackListAdaptor::emitTrackAdded(int track)
{
    Q_EMIT TrackAdded(metaData(track), QDBusObjectPath(((track > 0)?QString("/track_%1").arg(track - 1):"/org/mpris/MediaPlayer2/TrackList/NoTrack")));
}

void DBusTrackListAdaptor::emitTrackRemoved(int track)
{
    Q_EMIT TrackRemoved(QDBusObjectPath(QString("/track_%1").arg(track)));
}

void DBusTrackListAdaptor::emitTrackMetadataChanged(int track)
{
    Q_EMIT TrackMetadataChanged(QDBusObjectPath(QString("/track_%1").arg(track)), metaData(track));
}

QList<QVariantMap> DBusTrackListAdaptor::GetTracksMetadata(const QList<QDBusObjectPath> &trackIds) const
{
    QList<QVariantMap> metaData;

    if (!m_player->playlist())
    {
        return metaData;
    }

    for (int i = 0; i < trackIds.count(); ++i)
    {
        const int track = trackNumber(trackIds.at(i).path());

        if (track >= 0 && track < m_player->playlist()->trackCount())
        {
            metaData.append(this->metaData(track));
        }
    }

    return metaData;
}

QList<QDBusObjectPath> DBusTrackListAdaptor::Tracks() const
{
    QList<QDBusObjectPath> tracks;

    if (m_player->playlist())
    {
        for (int i = 0; i < m_player->playlist()->trackCount(); ++i)
        {
            tracks.append(QDBusObjectPath(QString("/track_%1").arg(i)));
        }
    }

    return tracks;
}

QVariantMap DBusTrackListAdaptor::metaData(int track) const
{
    QVariantMap metaData;

    if (!m_player->playlist() || track > m_player->playlist()->trackCount() || track < 0)
    {
        return metaData;
    }

    const KUrl url(m_player->playlist()->track(track));

    metaData["mpris:trackid"] = QString("/track_%1").arg(track);
    metaData["mpris:length"] = MetaDataManager::duration(url);
    metaData["xesam:url"] = url.pathOrUrl();
    metaData["xesam:title"] = MetaDataManager::metaData(url, TitleKey);
    metaData["xesam:artist"] = MetaDataManager::metaData(url, ArtistKey);
    metaData["xesam:album"] = MetaDataManager::metaData(url, AlbumKey);
    metaData["xesam:genre"] = QStringList(MetaDataManager::metaData(url, GenreKey));
    metaData["xesam:comment"] = QStringList(MetaDataManager::metaData(url, DescriptionKey));
    metaData["xesam:trackNumber"] = MetaDataManager::metaData(url, TrackNumberKey);

    return metaData;
}

int DBusTrackListAdaptor::trackNumber(const QString &trackId)
{
    if (!trackId.startsWith("/track_") || trackId.length() < 8)
    {
        return -1;
    }

    return trackId.mid(7).toInt();
}

bool DBusTrackListAdaptor::CanEditTracks() const
{
    return true;
}

}
