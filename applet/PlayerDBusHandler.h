/***********************************************************************************
* Copyright (C) 2008 by Marco Martin <notmart@gmail.com>
* Copyright (C) 2009 - 2012 by Michal Dutkiewicz <emdeck@gmail.com>
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

#ifndef PLAYERDBUSHANDLER_HEADER
#define PLAYERDBUSHANDLER_HEADER

#include <QtCore/QObject>
#include <QtCore/QVariantMap>
#include <QtDBus/QDBusArgument>

struct DBusStatus
{
    int Play;
    int Random;
    int Repeat;
    int RepeatPlaylist;
};

Q_DECLARE_METATYPE(DBusStatus)

QDBusArgument &operator << (QDBusArgument &argument, const DBusStatus &status);
const QDBusArgument &operator >> (const QDBusArgument &argument, DBusStatus &status);

enum DbusCaps
{
    NONE                  = 0,
    CAN_GO_NEXT           = 1 << 0,
    CAN_GO_PREV           = 1 << 1,
    CAN_PAUSE             = 1 << 2,
    CAN_PLAY              = 1 << 3,
    CAN_SEEK              = 1 << 4,
    CAN_PROVIDE_METADATA  = 1 << 5,
    CAN_HAS_TRACKLIST     = 1 << 6
};


namespace MiniPlayer
{

class Player;

}

class PlayerDBusHandler : public QObject
{
    Q_OBJECT

    public:
        PlayerDBusHandler(MiniPlayer::Player *parent);

    public Q_SLOTS:
        void PlayPause();
        void Play();
        void Pause();
        void Stop();
        void Next();
        void Prev();
        void Repeat(bool enable);
        void VolumeSet(int value);
        void PositionSet(int position);
        DBusStatus GetStatus();
        QVariantMap GetMetadata();
        int GetCaps();
        int VolumeGet() const;
        qint64 PositionGet() const;

    protected Q_SLOTS:
        void stateChanged();
        void trackChanged();
        void seekableChanged();

    Q_SIGNALS:
        void StatusChange(DBusStatus status);
        void CapsChange(int caps);
        void TrackChange(QVariantMap metadata);

    private:
        MiniPlayer::Player *m_player;
};

#endif
