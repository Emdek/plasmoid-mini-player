/***********************************************************************************
* Mini Player: Advanced media player for Plasma.
* Copyright (C) 2008 - 2013 Michal Dutkiewicz aka Emdek <emdeck@gmail.com>
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

#ifndef MINIPLAYERDBUSTRACKLISTADAPTOR
#define MINIPLAYERDBUSTRACKLISTADAPTOR

#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusAbstractAdaptor>

namespace MiniPlayer
{

class Player;

class DBusTrackListAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT

    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.TrackList")

    Q_PROPERTY(QList<QDBusObjectPath> Tracks READ Tracks)
    Q_PROPERTY(bool CanEditTracks READ CanEditTracks)

    public:
        explicit DBusTrackListAdaptor(QObject *parent, Player *player);

        QList<QVariantMap> GetTracksMetadata(const QList<QDBusObjectPath> &trackIds) const;
        QList<QDBusObjectPath> Tracks() const;
        bool CanEditTracks() const;

    public slots:
        void AddTrack(const QString &uri, const QDBusObjectPath &afterTrack, bool setAsCurrent) const;
        void RemoveTrack(const QDBusObjectPath &trackId) const;
        void GoTo(const QDBusObjectPath &trackId) const;

    protected:
        QVariantMap metaData(int track) const;
        static int trackNumber(const QString &trackId);

    protected slots:
        void emitTrackListReplaced();
        void emitTrackAdded(int track);
        void emitTrackRemoved(int track);
        void emitTrackMetadataChanged(int track);

    private:
        Player *m_player;

    signals:
        void TrackListReplaced(QList<QDBusObjectPath> trackIds, QDBusObjectPath currentTrack);
        void TrackAdded(QVariantMap metaData, QDBusObjectPath afterTrack);
        void TrackRemoved(QDBusObjectPath trackId);
        void TrackMetadataChanged(QDBusObjectPath trackId, QVariantMap metaData);
};

}

#endif
