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

#ifndef MINIPLAYERPLAYLISTWRITER_HEADER
#define MINIPLAYERPLAYLISTWRITER_HEADER

#include <QtCore/QObject>

#include <KUrl>

#include "Constants.h"

namespace MiniPlayer
{

class PlaylistModel;

class PlaylistWriter : public QObject
{
    Q_OBJECT

    public:
        explicit PlaylistWriter(QObject *parent, const QString &path, PlaylistModel *playlist, PlaylistFormat format = InvalidFormat);

    public:
        bool writePls(const QString &path, const KUrl::List &tracks);
        bool writeM3u(const QString &path, const KUrl::List &tracks);
        bool writeXspf(const QString &path, const PlaylistModel *playlist);
        bool writeAsx(const QString &path, const PlaylistModel *playlist);
        bool status() const;

    private:
        bool m_status;
};

}

#endif
