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

#include "PlaylistModel.h"
#include "Player.h"
#include "MetaDataManager.h"

#include <KIcon>
#include <KLocale>
#include <KMimeType>

namespace MiniPlayer
{

PlaylistModel::PlaylistModel(Player *parent) : QAbstractTableModel(parent),
    m_player(parent),
    m_playlist(new QMediaPlaylist(this))
{
    setSupportedDragActions(Qt::MoveAction);

//    connect(m_player, SIGNAL(resetModel()), this, SIGNAL(layoutChanged()));
    connect(m_playlist, SIGNAL(mediaChanged(int,int)), this, SIGNAL(layoutChanged()));
    connect(m_playlist, SIGNAL(mediaInserted(int,int)), this, SIGNAL(layoutChanged()));
    connect(m_playlist, SIGNAL(mediaRemoved(int,int)), this, SIGNAL(layoutChanged()));
    connect(m_playlist, SIGNAL(currentIndexChanged(int)), this, SIGNAL(layoutChanged()));
}

void PlaylistModel::addTracks(const KUrl::List &tracks, int index)
{
///FIXME remove? check validity in reader (moved into playlistmanager?)?
    QList<QMediaContent> items;

    if (index == -1)
    {
        index = m_playlist->mediaCount();
    }

    for (int i = 0; i < tracks.count(); ++i)
    {
        if (!tracks.at(i).isValid())
        {
            continue;
        }

        items.append(QMediaContent(tracks.at(i)));
    }

    m_playlist->insertMedia(index, items);

    emit needsSaving();
}

void PlaylistModel::sort(int column, Qt::SortOrder order)
{
    Q_UNUSED(column)

    if (m_playlist->mediaCount() < 2)
    {
        return;
    }

    QMultiMap<QString, QMediaContent> urlMap;
    QMultiMap<QString, QMediaContent> titleMap;
    QMultiMap<qint64, QMediaContent> lengthMap;
    QList<QMediaContent> tracks;
    KUrl url;

    switch (column)
    {
        case 1:
            for (int i = 0; i < m_playlist->mediaCount(); ++i)
            {
                url = KUrl(m_playlist->media(i).canonicalUrl());

                titleMap.insert(m_player->metaDataManager()->title(url), QMediaContent(url));
            }

            tracks = titleMap.values();
        break;
        case 2:
            for (int i = 0; i < m_playlist->mediaCount(); ++i)
            {
                url = KUrl(m_playlist->media(i).canonicalUrl());

                lengthMap.insert(m_player->metaDataManager()->duration(url), QMediaContent(url));
            }

            tracks = lengthMap.values();
        break;
        default:
            for (int i = 0; i < m_playlist->mediaCount(); ++i)
            {
                url = KUrl(m_playlist->media(i).canonicalUrl());

                urlMap.insert(url.pathOrUrl(), QMediaContent(url));
            }

            tracks = urlMap.values();
        break;
    }

    if (order == Qt::AscendingOrder)
    {
        QList<QMediaContent> items;

        for (int i = (tracks.count() - 1); i >= 0; --i)
        {
            items.append(tracks.at(i));
        }

        tracks = items;
    }

    m_playlist->clear();
    m_playlist->insertMedia(0, tracks);

    emit layoutChanged();
    emit needsSaving();
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || (index.row() >= m_playlist->mediaCount()))
    {
        return QVariant();
    }

    KUrl url(m_playlist->media(index.row()).canonicalUrl());

    if (role == Qt::DecorationRole && index.column() == 0 && url.isValid())
    {
        return ((m_player->playlist() == m_playlist && index.row() == m_playlist->currentIndex())?KIcon("media-playback-start"):m_player->metaDataManager()->icon(url));
    }
    else if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column())
        {
            case 1:
                return m_player->metaDataManager()->title(url);
            case 2:
                return MetaDataManager::timeToString(m_player->metaDataManager()->duration(url));
        }
    }
    else if (role == Qt::ToolTipRole)
    {
        return url.pathOrUrl();
    }

    return QVariant();
}

QVariant PlaylistModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
    {
        return QVariant();
    }

    switch (section)
    {
        case 1:
            return i18n("Title");
        case 2:
            return i18n("Length");
    }

    return QVariant();
}

Qt::ItemFlags PlaylistModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractTableModel::flags(index);

    if (index.isValid())
    {
        if (index.column() == 1)
        {
            return (defaultFlags | Qt::ItemIsDragEnabled | Qt::ItemIsEditable);
        }
        else
        {
            return (defaultFlags | Qt::ItemIsDragEnabled);
        }
    }
    else
    {
        return (defaultFlags | Qt::ItemIsDropEnabled);
    }
}

Qt::DropActions PlaylistModel::supportedDropActions() const
{
    return (Qt::CopyAction | Qt::MoveAction);
}

QMimeData* PlaylistModel::mimeData(const QModelIndexList &indexes) const
{
    KUrl::List urls;

    foreach (const QModelIndex &index, indexes)
    {
        if (index.isValid() && (index.column() == 0))
        {
            urls.append(KUrl(m_playlist->media(index.row()).canonicalUrl()));
        }
    }

    QMimeData *mimeData = new QMimeData();
    urls.populateMimeData(mimeData);

    return mimeData;
}

QMediaPlaylist* PlaylistModel::playlist()
{
    return m_playlist;
}

QStringList PlaylistModel::mimeTypes() const
{
    return QStringList("text/uri-list");
}

int PlaylistModel::columnCount(const QModelIndex &index) const
{
    if (index.isValid())
    {
        return 0;
    }

    return 3;
}

int PlaylistModel::rowCount(const QModelIndex &index) const
{
    if (index.isValid())
    {
        return 0;
    }

    return m_playlist->mediaCount();
}

bool PlaylistModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || value.toString().isEmpty() || index.row() >= m_playlist->mediaCount())
    {
        return false;
    }

    if (role == Qt::EditRole)
    {
        m_player->metaDataManager()->setMetaData(KUrl(m_playlist->media(index.row()).canonicalUrl()), value.toString(), -1);
    }
    else
    {
        return false;
    }

    emit needsSaving();
    emit itemChanged(index);

    return true;
}

bool PlaylistModel::dropMimeData(const QMimeData *mimeData, Qt::DropAction action, int row, int column, const QModelIndex &index)
{
    Q_UNUSED(column)

    if (action == Qt::IgnoreAction)
    {
        return true;
    }

    if (!mimeData->hasUrls())
    {
        return false;
    }

    int position = m_playlist->mediaCount();

    if (index.isValid())
    {
        position = index.row();
    }
    else if (row != -1)
    {
        position = row;
    }

    KUrl::List urls = KUrl::List::fromMimeData(mimeData);

///FIXME move here or to PlaylistManager
//     m_player->addToPlaylist(urls, false, position);

    emit needsSaving();

    return true;
}

bool PlaylistModel::insertRows(int row, int count, const QModelIndex &index)
{
    if (!index.isValid() || row < 0 || row >= m_playlist->mediaCount())
    {
        return false;
    }

    int end = (row + count);

    beginInsertRows(index, row, (end - 1));

    for (int i = 0; i < count; ++i)
    {
        m_playlist->insertMedia((row + i), QMediaContent());
    }

    endInsertRows();

    emit needsSaving();

    return true;
}

bool PlaylistModel::removeRows(int row, int count, const QModelIndex &index)
{
    if (!index.isValid() || row < 0 || row >= m_playlist->mediaCount())
    {
        return false;
    }

    int end = (row + count);

    beginRemoveRows(index, row, (end - 1));

    m_playlist->removeMedia(row, end);

    endRemoveRows();

    emit needsSaving();

    return true;
}

}
