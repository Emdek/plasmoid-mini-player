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

#ifndef MINIPLAYERPLAYLISTMODEL_HEADER
#define MINIPLAYERPLAYLISTMODEL_HEADER

#include <QtCore/QHash>
#include <QtCore/QVariant>
#include <QtCore/QMimeData>
#include <QtCore/QAbstractTableModel>

#include <KUrl>
#include <KIcon>

#include "Constants.h"

namespace MiniPlayer
{

class PlaylistManager;

class PlaylistModel : public QAbstractTableModel
{
    Q_OBJECT

    public:
        PlaylistModel(PlaylistManager *parent, const QString &title, PlaylistSource source = LocalSource);

        void addTrack(int position, const KUrl &url);
        void removeTrack(int position);
        void addTracks(const KUrl::List &tracks, int position = -1, bool play = false);
        void sort(int column, Qt::SortOrder order);
        void setTitle(const QString &title);
        QStringList mimeTypes() const;
        QString title() const;
        KIcon icon() const;
        QVariant data(const QModelIndex &index, int role) const;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const;
        QMimeData* mimeData(const QModelIndexList &indexes) const;
        Qt::ItemFlags flags(const QModelIndex &index) const;
        Qt::DropActions supportedDropActions() const;
        KUrl track(int position) const;
        PlaybackMode playbackMode() const;
        PlaylistSource source() const;
        int currentTrack() const;
        int nextTrack() const;
        int trackCount() const;
        int columnCount(const QModelIndex &index) const;
        int rowCount(const QModelIndex &index) const;
        bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
        bool dropMimeData(const QMimeData *mimeData, Qt::DropAction action, int row, int column, const QModelIndex &index);
        bool insertRows(int row, int count, const QModelIndex &index = QModelIndex());
        bool removeRows(int row, int count, const QModelIndex &index = QModelIndex());
        bool isReadOnly() const;

    public slots:
        void clear();
        void shuffle();
        void next();
        void previous();
        void setCurrentTrack(int track, bool play = false);
        void setPlaybackMode(PlaybackMode mode);

    protected:
        int randomTrack() const;

    protected slots:
        void addTracks(const KUrl::List &tracks, const QHash<KUrl, QPair<QString, qint64> > &metaData, int position, bool play);

    private:
        PlaylistManager *m_manager;
        KUrl::List m_tracks;
        QString m_title;
        PlaybackMode m_playbackMode;
        PlaylistSource m_source;
        int m_currentTrack;

    signals:
        void needsSaving();
        void currentTrackChanged(int track, bool play);

    friend class Player;
};

}

#endif
