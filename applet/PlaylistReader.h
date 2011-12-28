/***********************************************************************************
* Mini Player: Advanced media player for Plasma.
* Copyright (C) 2008 - 2011 Michal Dutkiewicz aka Emdek <emdeck@gmail.com>
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

#ifndef MINIPLAYERPLAYLISTREADER_HEADER
#define MINIPLAYERPLAYLISTREADER_HEADER

#include <QtCore/QObject>
#include <QtCore/QTextStream>

#include <KIO/Job>
#include <KIO/NetAccess>

#include "Constants.h"

namespace MiniPlayer
{

class PlaylistModel;

class PlaylistReader : public QObject
{
    Q_OBJECT

    public:
        PlaylistReader(PlaylistModel *parent, const KUrl::List &urls, int index, bool play);

    public slots:
        void importData(KIO::Job *job, const QByteArray &data);
        void importResult(KJob *job);

    protected:
        void addUrls(const KUrl::List &items, int level = 0);
        void importPlaylist(const KUrl &url, PlaylistType type);
        void readM3u(QTextStream &stream);
        void readPls(QTextStream &stream);
        void readXspf(QByteArray &data);
        void readAsx(QByteArray &data);
        void readDirectory(const KUrl &url, int level = 0);

    private:
        QHash<KJob*, QByteArray> m_remotePlaylistsData;
        QHash<KJob*, PlaylistType> m_remotePlaylistsType;
        QHash<KUrl, QPair<QString, qint64> > m_metaData;
        KUrl::List m_tracks;
        int m_imports;
        int m_index;
        bool m_play;

    signals:
        void processedTracks(KUrl::List tracks, QHash<KUrl, QPair<QString, qint64> > metaData, int index, bool play);
};

}

#endif
