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
#include "PlaylistReader.h"
#include "Player.h"
#include "MetaDataManager.h"

#include <QtCore/QDateTime>

#include <KLocale>
#include <KMimeType>
#include <KRandomSequence>

namespace MiniPlayer
{

PlaylistModel::PlaylistModel(Player *parent, const QString &title, PlaylistSource source) : QAbstractTableModel(parent),
    m_player(parent),
    m_title(title),
    m_playbackMode(SequentialMode),
    m_source(source),
    m_currentTrack(-1)
{
    setSupportedDragActions(Qt::MoveAction);
    setPlaybackMode(m_playbackMode);

    connect(this, SIGNAL(needsSaving()), this, SIGNAL(layoutChanged()));
    connect(m_player->parent(), SIGNAL(resetModel()), this, SIGNAL(layoutChanged()));
}

void PlaylistModel::addTrack(int position, const KUrl &url)
{
    m_tracks.insert(position, url);

    emit needsSaving();
}

void PlaylistModel::removeTrack(int position)
{
    m_tracks.removeAt(position);

    emit needsSaving();
}

void PlaylistModel::addTracks(const KUrl::List &tracks, int position, bool play)
{
    if (position == -1)
    {
        position = m_tracks.count();
    }

    new PlaylistReader(this, tracks, position, play);
}

void PlaylistModel::addTracks(const KUrl::List &tracks, const QHash<KUrl, QPair<QString, qint64> > &metaData, int position, bool play)
{
    for (int i = (tracks.count() - 1); i >= 0; --i)
    {
        if (tracks.at(i).isValid())
        {
            m_tracks.insert(position, tracks.at(i));
        }
    }

    m_player->metaDataManager()->addTracks(tracks);

    if (trackCount())
    {
        setCurrentTrack(position, play);
    }

    m_player->metaDataManager()->setMetaData(metaData);

    emit needsSaving();
}

void PlaylistModel::clear()
{
    m_tracks.clear();

    emit needsSaving();
}

void PlaylistModel::shuffle()
{
    KRandomSequence().randomize(m_tracks);

    emit needsSaving();
}

void PlaylistModel::sort(int column, Qt::SortOrder order)
{
    Q_UNUSED(column)

    if (m_tracks.count() < 2)
    {
        return;
    }

    QMultiMap<QString, KUrl> urlMap;
    QMultiMap<QString, KUrl> titleMap;
    QMultiMap<qint64, KUrl> lengthMap;
    KUrl::List tracks;

    switch (column)
    {
        case 1:
            for (int i = 0; i < m_tracks.count(); ++i)
            {
                titleMap.insert(m_player->metaDataManager()->title(m_tracks.at(i)), m_tracks.at(i));
            }

            tracks = titleMap.values();
        break;
        case 2:
            for (int i = 0; i < m_tracks.count(); ++i)
            {
                lengthMap.insert(m_player->metaDataManager()->duration(m_tracks.at(i)), m_tracks.at(i));
            }

            tracks = lengthMap.values();
        break;
        default:
            for (int i = 0; i < m_tracks.count(); ++i)
            {
                urlMap.insert(m_tracks.at(i).pathOrUrl(), m_tracks.at(i));
            }

            tracks = urlMap.values();
        break;
    }

    if (order == Qt::AscendingOrder)
    {
        KUrl::List items;

        for (int i = (tracks.count() - 1); i >= 0; --i)
        {
            items.append(tracks.at(i));
        }

        tracks = items;
    }

    m_tracks = tracks;

    emit needsSaving();
}

void PlaylistModel::next()
{
    if (m_tracks.count() < 2)
    {
        setCurrentTrack(0);
    }
    else if (m_playbackMode == RandomMode)
    {
        setCurrentTrack(randomTrack());
    }
    else
    {
        if (m_currentTrack >= (m_tracks.count() - 1))
        {
            setCurrentTrack(0);
        }
        else
        {
            setCurrentTrack(m_currentTrack + 1);
        }
    }
}

void PlaylistModel::previous()
{
    if (m_tracks.count() < 2)
    {
        setCurrentTrack(0);
    }
    else if (m_playbackMode == RandomMode)
    {
        setCurrentTrack(randomTrack());
    }
    else
    {
        if (m_currentTrack == 0)
        {
            setCurrentTrack(m_tracks.count() - 1);
        }
        else
        {
            setCurrentTrack(m_currentTrack - 1);
        }
    }
}

void PlaylistModel::setCurrentTrack(int track, bool play)
{
    if (track > m_tracks.count())
    {
        track = 0;
    }

    if (m_currentTrack != track)
    {
        m_currentTrack = track;

        emit currentTrackChanged(m_currentTrack, play);
    }
}

void PlaylistModel::setPlaybackMode(PlaybackMode mode)
{
    m_playbackMode = mode;

    emit needsSaving();
}

void PlaylistModel::setTitle(const QString &title)
{
    m_title = title;
}

QString PlaylistModel::title() const
{
    return m_title;
}

KIcon PlaylistModel::icon() const
{
    switch (m_source)
    {
        case DvdSource:
            return KIcon("media-optical-dvd");
        case VcdSource:
            return KIcon("media-optical");
        case CdSource:
            return KIcon("media-optical-audio");
        default:
            return KIcon("view-media-playlist");
    }

    return KIcon("view-media-playlist");
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || (index.row() >= m_tracks.count()))
    {
        return QVariant();
    }

    KUrl url(m_tracks.at(index.row()));

    if (role == Qt::DecorationRole && index.column() == 0 && url.isValid())
    {
        return ((m_player->playlist() == this && index.row() == m_currentTrack)?KIcon((m_player->state() == StoppedState)?"arrow-right":"media-playback-start"):m_player->metaDataManager()->icon(url));
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
            urls.append(KUrl(m_tracks.at(index.row())));
        }
    }

    QMimeData *mimeData = new QMimeData();
    urls.populateMimeData(mimeData);

    return mimeData;
}

QStringList PlaylistModel::mimeTypes() const
{
    return QStringList("text/uri-list");
}

KUrl PlaylistModel::track(int position) const
{
    return m_tracks.value(position, KUrl());
}

PlaybackMode PlaylistModel::playbackMode() const
{
    return m_playbackMode;
}

PlaylistSource PlaylistModel::source() const
{
    return m_source;
}

int PlaylistModel::randomTrack() const
{
    if (trackCount() < 2)
    {
        return 0;
    }

    qsrand(QDateTime::currentDateTime().toTime_t());

    KRandomSequence sequence(qrand() % 1000);
    int randomTrack = currentTrack();

    while (randomTrack == currentTrack())
    {
        randomTrack = sequence.getLong(trackCount() - 1);
    }

    return randomTrack;
}

int PlaylistModel::currentTrack() const
{
    return m_currentTrack;
}

int PlaylistModel::nextTrack() const
{
    if (m_tracks.isEmpty())
    {
        return -1;
    }

    switch (m_playbackMode)
    {
        case RandomMode:
            return randomTrack();
        case LoopTrackMode:
            return m_currentTrack;
        case LoopPlaylistMode:
        default:
            if ((m_currentTrack + 1) >= m_tracks.count())
            {
                return ((m_playbackMode == LoopPlaylistMode)?0:-1);
            }

            return (m_currentTrack + 1);
    }

    return -1;
}

int PlaylistModel::trackCount() const
{
    return m_tracks.count();
}

int PlaylistModel::columnCount(const QModelIndex &index) const
{
    return (index.isValid()?0:3);
}

int PlaylistModel::rowCount(const QModelIndex &index) const
{
    return (index.isValid()?0:m_tracks.count());
}

bool PlaylistModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || value.toString().isEmpty() || index.row() >= m_tracks.count())
    {
        return false;
    }

    if (role == Qt::EditRole)
    {
        m_player->metaDataManager()->setMetaData(m_tracks.at(index.row()), value.toString(), -1);
    }
    else
    {
        return false;
    }

    emit needsSaving();

    return true;
}

bool PlaylistModel::dropMimeData(const QMimeData *mimeData, Qt::DropAction action, int row, int column, const QModelIndex &index)
{
    Q_UNUSED(column)

    if (action == Qt::IgnoreAction)
    {
        return true;
    }

    if (isReadOnly() || !mimeData->hasUrls())
    {
        return false;
    }

    int position = m_tracks.count();

    if (index.isValid())
    {
        position = index.row();
    }
    else if (row != -1)
    {
        position = row;
    }

    addTracks(KUrl::List::fromMimeData(mimeData), position, false);

    return true;
}

bool PlaylistModel::insertRows(int row, int count, const QModelIndex &index)
{
    if (!index.isValid() || row < 0 || row >= m_tracks.count())
    {
        return false;
    }

    int end = (row + count);

    beginInsertRows(index, row, (end - 1));

    for (int i = 0; i < count; ++i)
    {
        m_tracks.insert((row + i), KUrl());
    }

    endInsertRows();

    emit needsSaving();

    return true;
}

bool PlaylistModel::removeRows(int row, int count, const QModelIndex &index)
{
    if (!index.isValid() || row < 0 || row >= m_tracks.count())
    {
        return false;
    }

    int end = (row + count);

    beginRemoveRows(index, row, (end - 1));

    for (int i = 0; i < (end - row); ++i)
    {
        m_tracks.removeAt(row);
    }

    endRemoveRows();

    emit needsSaving();

    return true;
}

bool PlaylistModel::isReadOnly() const
{
    return (m_source != LocalSource);
}

}
