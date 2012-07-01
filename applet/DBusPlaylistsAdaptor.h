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

#ifndef MINIPLAYERDBUSPLAYLISTSADAPTOR
#define MINIPLAYERDBUSPLAYLISTSADAPTOR

#include <QtCore/QVariantMap>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusAbstractAdaptor>

namespace MiniPlayer
{

class PlaylistManager;

class DBusPlaylistsAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT

    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Playlists")

    Q_PROPERTY(int PlaylistCount READ PlaylistCount)
    Q_PROPERTY(QStringList Orderings READ Orderings)
    Q_PROPERTY(QVariantMap ActivePlaylist READ ActivePlaylist)

    public:
        explicit DBusPlaylistsAdaptor(QObject *parent, PlaylistManager *playlistManager);

        QVariantMap ActivePlaylist() const;
        QList<QVariantMap> GetPlaylists(int index, int maxCount, QString order, bool reverseOrder) const;
        QStringList Orderings() const;
        int PlaylistCount() const;

    public Q_SLOTS:
        void ActivatePlaylist(const QDBusObjectPath &playlistId) const;

    protected Q_SLOTS:
        void emitPlaylistCountChanged();
        void emitActivePlaylistChanged();
        void emitPlaylistChanged(int id);

    private:
        PlaylistManager *m_playlistManager;

    Q_SIGNALS:
        void PlaylistChanged(QVariantMap playlist);
};

}

#endif
