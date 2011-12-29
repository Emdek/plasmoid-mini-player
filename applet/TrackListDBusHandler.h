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

#ifndef TRACKLIST_DBUS_HANDLER_H
#define TRACKLIST_DBUS_HANDLER_H

#include <QtCore/QVariantMap>

namespace MiniPlayer
{

class Player;

}

class TrackListDBusHandler : public QObject
{
    Q_OBJECT

    public:
        TrackListDBusHandler(MiniPlayer::Player *parent);

    public Q_SLOTS:
        void SetLoop(bool enable);
        void SetRandom(bool enable);
        void DelTrack(int index);
        int AddTrack(const QString &url, bool playImmediately);
        int GetCurrentTrack();
        int GetLength();
        QVariantMap GetMetadata(int index);

    Q_SIGNALS:
        void TrackListChange(int count);

    private:
        MiniPlayer::Player *m_player;
};

#endif
