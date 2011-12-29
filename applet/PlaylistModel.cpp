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
#include "PlaylistManager.h"
#include "MetaDataManager.h"

#include <QtCore/QDateTime>

#include <KLocale>
#include <KMimeType>
#include <KRandomSequence>

namespace MiniPlayer
{

PlaylistModel::PlaylistModel(PlaylistManager *parent, const QString &title, PlaylistSource source) : QAbstractTableModel(parent),
    m_manager(parent),
    m_title(title),
    m_playbackMode(SequentialMode),
    m_source(source),
    m_currentTrack(-1)
{
    setSupportedDragActions(Qt::MoveAction);
    setPlaybackMode(m_playbackMode);

    connect(this, SIGNAL(needsSaving()), this, SIGNAL(layoutChanged()));
    connect(MetaDataManager::instance(), SIGNAL(urlChanged(KUrl)), this, SIGNAL(layoutChanged()));
}

void PlaylistModel::addTrack(int position, const KUrl &url)
{
    m_tracks.insert(position, url);

    if (position <= m_currentTrack)
    {
        setCurrentTrack(qMin((position + 1), (m_tracks.count() - 1)));
    }

    emit needsSaving();
}

void PlaylistModel::removeTrack(int position)
{
    m_tracks.removeAt(position);

    if (position == m_currentTrack && (m_manager->state() != StoppedState && m_manager->playlists().value(m_manager->currentPlaylist()) == this))
    {
        setCurrentTrack(qMax(0, (m_tracks.count() - 1)), StopReaction);
    }
    else if (position < m_currentTrack)
    {
        setCurrentTrack(m_currentTrack - 1);
    }

    emit needsSaving();
}

void PlaylistModel::addTracks(const KUrl::List &tracks, int position, PlayerReaction reaction)
{
    if (position == -1)
    {
        position = m_tracks.count();
    }

    new PlaylistReader(this, tracks, position, reaction);
}

void PlaylistModel::addTracks(const KUrl::List &tracks, const QHash<KUrl, QPair<QString, qint64> > &metaData, int position, PlayerReaction reaction)
{
    for (int i = (tracks.count() - 1); i >= 0; --i)
    {
        m_tracks.insert(position, tracks.at(i));
    }

    if (reaction == PlayReaction)
    {
        setCurrentTrack(position, reaction);
    }
    else if (position <= m_currentTrack)
    {
        if (m_tracks.count() == tracks.count())
        {
            setCurrentTrack(0, reaction);
        }
        else
        {
            setCurrentTrack(qMin((m_currentTrack + tracks.count()), (m_tracks.count() - 1)), reaction);
        }
    }

    MetaDataManager::setMetaData(metaData);
    MetaDataManager::addTracks(tracks);

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
                titleMap.insert(MetaDataManager::title(m_tracks.at(i)), m_tracks.at(i));
            }

            tracks = titleMap.values();
        break;
        case 2:
            for (int i = 0; i < m_tracks.count(); ++i)
            {
                lengthMap.insert(MetaDataManager::duration(m_tracks.at(i)), m_tracks.at(i));
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

void PlaylistModel::next(PlayerReaction reaction)
{
    if (m_tracks.count() < 2)
    {
        setCurrentTrack(0, reaction);
    }
    else if (m_playbackMode == RandomMode)
    {
        setCurrentTrack(randomTrack(), reaction);
    }
    else
    {
        setCurrentTrack(((m_currentTrack >= (m_tracks.count() - 1))?0:(m_currentTrack + 1)), reaction);
    }
}

void PlaylistModel::previous(PlayerReaction reaction)
{
    if (m_tracks.count() < 2)
    {
        setCurrentTrack(0, reaction);
    }
    else if (m_playbackMode == RandomMode)
    {
        setCurrentTrack(randomTrack(), reaction);
    }
    else
    {
        setCurrentTrack(((m_currentTrack == 0)?(m_tracks.count() - 1):(m_currentTrack - 1)), reaction);
    }
}

void PlaylistModel::setCurrentTrack(int track, PlayerReaction reaction)
{
    if (track > m_tracks.count())
    {
        track = 0;
    }

    m_currentTrack = track;

    emit currentTrackChanged(m_currentTrack, reaction);
    emit layoutChanged();
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
        return ((index.row() == m_currentTrack)?KIcon((m_manager->state() != StoppedState && m_manager->playlists().value(m_manager->currentPlaylist()) == this)?"media-playback-start":"arrow-right"):MetaDataManager::icon(url));
    }
    else if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column())
        {
            case 1:
                return MetaDataManager::title(url);
            case 2:
                return MetaDataManager::timeToString(MetaDataManager::duration(url));
        }
    }
    else if (role == Qt::ToolTipRole)
    {
        return url.pathOrUrl();
    }
    else if (role == Qt::UserRole)
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

QStringList PlaylistModel::mimeTypes() const
{
    return QStringList("text/uri-list");
}

KUrl::List PlaylistModel::tracks() const
{
    return m_tracks;
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
        MetaDataManager::setMetaData(m_tracks.at(index.row()), value.toString(), -1);
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

    addTracks(KUrl::List::fromMimeData(mimeData), position, NoReaction);

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
