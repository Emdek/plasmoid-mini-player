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
#include <QtMultimediaKit/QMediaPlaylist>

#include <KUrl>

namespace MiniPlayer
{

class Player;

class PlaylistModel : public QAbstractTableModel
{
    Q_OBJECT

    public:
        PlaylistModel(Player *parent, const QString &title);

        void addTrack(int position, const KUrl &url);
        void removeTrack(int position);
        void addTracks(const KUrl::List &tracks, int index = -1, bool play = false);
        void sort(int column, Qt::SortOrder order);
        void setTitle(const QString &title);
        QStringList mimeTypes() const;
        QString title() const;
        QVariant data(const QModelIndex &index, int role) const;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const;
        QMimeData* mimeData(const QModelIndexList &indexes) const;
        Qt::ItemFlags flags(const QModelIndex &index) const;
        Qt::DropActions supportedDropActions() const;
        KUrl track(int position) const;
        int currentTrack() const;
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
        void setCurrentTrack(int track);

    protected:
        QMediaPlaylist* playlist();

    protected slots:
        void addTracks(const KUrl::List &tracks, const QHash<KUrl, QPair<QString, qint64> > &metaData, int index, bool play);

    private:
        Player *m_player;
        QMediaPlaylist *m_playlist;
        QString m_title;

    signals:
        void needsSaving();
        void itemChanged(QModelIndex index);

    friend class Player;
};

}

#endif
