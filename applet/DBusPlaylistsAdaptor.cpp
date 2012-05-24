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

#include "DBusPlaylistsAdaptor.h"
#include "DBusInterface.h"
#include "PlaylistManager.h"
#include "PlaylistModel.h"

namespace MiniPlayer
{

DBusPlaylistsAdaptor::DBusPlaylistsAdaptor(QObject *parent, PlaylistManager *playlistManager) : QDBusAbstractAdaptor(parent),
    m_playlistManager(playlistManager)
{
    connect(m_playlistManager, SIGNAL(playlistAdded(int)), this, SLOT(emitPlaylistCountChanged()));
    connect(m_playlistManager, SIGNAL(playlistRemoved(int)), this, SLOT(emitPlaylistCountChanged()));
    connect(m_playlistManager, SIGNAL(currentPlaylistChanged(int)), this, SLOT(emitActivePlaylistChanged()));
    connect(m_playlistManager, SIGNAL(playlistChanged(int)), this, SLOT(emitPlaylistChanged(int)));
}

void DBusPlaylistsAdaptor::ActivatePlaylist(const QDBusObjectPath &playlistId) const
{
    if (!playlistId.path().startsWith("/playlist_") || playlistId.path().length() < 11)
    {
        return;
    }

    const int playlist = playlistId.path().mid(10).toInt();

    if (playlist >= 0 && playlist < m_playlistManager->playlists().count())
    {
        m_playlistManager->setCurrentPlaylist(playlist);
    }
}

void DBusPlaylistsAdaptor::emitPlaylistCountChanged()
{
    QVariantMap properties;
    properties["PlaylistCount"] = PlaylistCount();

    DBusInterface::updateProperties("org.mpris.MediaPlayer2.Playlists", properties);
}

void DBusPlaylistsAdaptor::emitActivePlaylistChanged()
{
    QVariantMap properties;
    properties["ActivePlaylist"] = ActivePlaylist();

    DBusInterface::updateProperties("org.mpris.MediaPlayer2.Playlists", properties);
}

void DBusPlaylistsAdaptor::emitPlaylistChanged(int id)
{
    PlaylistModel *changedPlaylist = m_playlistManager->playlist(id);

    if (!changedPlaylist)
    {
        return;
    }

    QVariantMap playlistData;
    playlistData["Id"] = qVariantFromValue(QDBusObjectPath(QString("/playlist_%1").arg(id)));
    playlistData["Name"] = changedPlaylist->title();

    emit PlaylistChanged(playlistData);
}

QVariantMap DBusPlaylistsAdaptor::ActivePlaylist() const
{
    PlaylistModel *playlist = m_playlistManager->playlist(m_playlistManager->currentPlaylist());
    QVariantMap activePlaylist;
    activePlaylist["Valid"] = (playlist != NULL);

    if (playlist)
    {
        QVariantMap playlistData;
        playlistData["Id"] = qVariantFromValue(QDBusObjectPath(QString("/playlist_%1").arg(m_playlistManager->currentPlaylist())));
        playlistData["Name"] = playlist->title();

        activePlaylist["Playlist"] = playlistData;
    }

    return activePlaylist;
}

QList<QVariantMap> DBusPlaylistsAdaptor::GetPlaylists(int index, int maxCount, QString order, bool reverseOrder) const
{
    QList<QVariantMap> reuestedPlaylists;
    QList<int> sortedPlaylists;
    const QList<int> playlists = m_playlistManager->playlists();

    if (order == "Alphabetical")
    {
        QMultiMap<QString, int> alphabeticalMap;

        for (int i = 0; i < playlists.count(); ++i)
        {
            PlaylistModel *playlist = m_playlistManager->playlist(playlists.at(i));

            alphabeticalMap.insert((playlist?playlist->title():QString()), i);
        }

        sortedPlaylists = alphabeticalMap.values();
    }
    else if (order == "CreationDate" || order == "ModifiedDate" || order == "LastPlayDate")
    {
        QMultiMap<QDateTime, int> datesMap;

        if (order == "CreationDate")
        {
            for (int i = 0; i < playlists.count(); ++i)
            {
                PlaylistModel *playlist = m_playlistManager->playlist(playlists.at(i));

                datesMap.insert((playlist?playlist->creationDate():QDateTime()), i);
            }
        }
        else if (order == "ModifiedDate")
        {
            for (int i = 0; i < playlists.count(); ++i)
            {
                PlaylistModel *playlist = m_playlistManager->playlist(playlists.at(i));

                datesMap.insert((playlist?playlist->modificationDate():QDateTime()), i);
            }
        }
        else
        {
            for (int i = 0; i < playlists.count(); ++i)
            {
                PlaylistModel *playlist = m_playlistManager->playlist(playlists.at(i));

                datesMap.insert((playlist?playlist->lastPlayedDate():QDateTime()), i);
            }
        }

        sortedPlaylists = datesMap.values();
    }
    else
    {
        for (int i = 0; i < playlists.count(); ++i)
        {
            sortedPlaylists.append(i);
        }
    }

    if (index < 0 || index >= sortedPlaylists.count())
    {
        index = 0;
    }

    if (maxCount < 0 || maxCount >= sortedPlaylists.count())
    {
        maxCount = sortedPlaylists.count();
    }

    if (reverseOrder)
    {
        QList<int> reversedPlaylists;

        for (int i = (sortedPlaylists.count() - 1); i >= 0; --i)
        {
            reversedPlaylists.append(sortedPlaylists.at(i));
        }

        sortedPlaylists = reversedPlaylists;
    }

    for (int i = index; i < maxCount; ++i)
    {
        PlaylistModel *playlist = m_playlistManager->playlist(playlists.at(i));
        QVariantMap playlistData;
        playlistData["Id"] = qVariantFromValue(QDBusObjectPath(QString("/playlist_%1").arg(sortedPlaylists.at(i))));
        playlistData["Name"] = (playlist?playlist->title():QString());

        reuestedPlaylists.append(playlistData);
    }

    return reuestedPlaylists;
}

QStringList DBusPlaylistsAdaptor::Orderings() const
{
    QStringList orderings;
    orderings << "Alphabetical" << "CreationDate" << "ModifiedDate" << "LastPlayDate" << "UserDefined";

    return orderings;
}

int DBusPlaylistsAdaptor::PlaylistCount() const
{
    return m_playlistManager->playlists().count();
}

}
