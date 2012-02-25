/***********************************************************************************
* Mini Player: Advanced media player for Plasma.
* Copyright (C) 2008 - 2012 Michal Dutkiewicz aka Emdek <emdeck@gmail.com>
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
    if (position < 0 || position >= m_tracks.count())
    {
        return;
    }

    m_manager->removeTracks(KUrl::List(m_tracks.at(position)));

    m_tracks.removeAt(position);

    if (position <= m_currentTrack)
    {
        setCurrentTrack((m_currentTrack - 1), ((position == m_currentTrack && (m_manager->state() != StoppedState && isCurrent()))?StopReaction:NoReaction));
    }
    else
    {
        setCurrentTrack(m_currentTrack);
    }
}

void PlaylistModel::addTracks(const KUrl::List &tracks, int position, PlayerReaction reaction)
{
    if (position == -1)
    {
        position = m_tracks.count();
    }

    new PlaylistReader(this, tracks, position, reaction);
}

void PlaylistModel::processedTracks(const KUrl::List &tracks, int position, PlayerReaction reaction)
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

    MetaDataManager::resolveTracks(tracks);

    emit needsSaving();
}

void PlaylistModel::clear()
{
    if (m_tracks.count() < 1)
    {
        return;
    }

    m_manager->removeTracks(m_tracks);

    m_tracks.clear();

    emit needsSaving();
}

void PlaylistModel::shuffle()
{
    if (m_tracks.count() < 2)
    {
        return;
    }

    KUrl url = m_tracks.value(m_currentTrack);

    KRandomSequence().randomize(m_tracks);

    setCurrentTrack(findTrack(url));
}

void PlaylistModel::sort(int column, Qt::SortOrder order)
{
    Q_UNUSED(column)

    if (m_tracks.count() < 2)
    {
        return;
    }

    QMultiMap<QString, KUrl> keyMap;
    QMultiMap<qint64, KUrl> durationMap;
    KUrl::List tracks;
    KUrl url = m_tracks.value(m_currentTrack);

    if (column == 8)
    {
        for (int i = 0; i < m_tracks.count(); ++i)
        {
            durationMap.insert(MetaDataManager::duration(m_tracks.at(i)), m_tracks.at(i));
        }

        tracks = durationMap.values();
    }
    else if (column > 0 && column << 8)
    {
        const MetaDataKey key = translateColumn(column);

        for (int i = 0; i < m_tracks.count(); ++i)
        {
            keyMap.insert(MetaDataManager::metaData(m_tracks.at(i), key), m_tracks.at(i));
        }

        tracks = keyMap.values();
    }
    else
    {
        for (int i = 0; i < m_tracks.count(); ++i)
        {
            keyMap.insert(m_tracks.at(i).pathOrUrl(), m_tracks.at(i));
        }

        tracks = keyMap.values();
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

    setCurrentTrack(findTrack(url));
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
    if (track > (m_tracks.count() - 1))
    {
        track = 0;
    }

    m_currentTrack = track;

    if (m_currentTrack == -1 && m_playbackMode == SequentialMode)
    {
        reaction = StopReaction;
        m_currentTrack = 0;
    }

    emit currentTrackChanged(m_currentTrack, reaction);
    emit needsSaving();
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
        return ((index.row() == m_currentTrack)?KIcon((m_manager->state() != StoppedState && isCurrent())?"media-playback-start":"arrow-right"):MetaDataManager::icon(url));
    }
    else if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        if (index.column() == 0)
        {
            return url.pathOrUrl();
        }
        else if (index.column() == 8)
        {
            return MetaDataManager::timeToString(MetaDataManager::duration(url));
        }
        else
        {
            return MetaDataManager::metaData(url, translateColumn(index.column()));
        }
    }
    else if (role == Qt::ToolTipRole)
    {
        return ((MetaDataManager::duration(url) > 0)?QString("<nobr>%1 - %2 (%3)</nobr>").arg(MetaDataManager::metaData(url, ArtistKey)).arg(MetaDataManager::metaData(url, TitleKey)).arg(MetaDataManager::timeToString(MetaDataManager::duration(url))):QString("<nobr>%1 - %2</nobr>").arg(MetaDataManager::metaData(url, ArtistKey)).arg(MetaDataManager::metaData(url, TitleKey)));
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
            return i18n("Artist");
        case 2:
            return i18n("Title");
        case 3:
            return i18n("Album");
        case 4:
            return i18n("Track Number");
        case 5:
            return i18n("Genre");
        case 6:
            return i18n("Description");
        case 7:
            return i18n("Date");
        case 8:
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
        if (index.column() > 0 && index.column() < 8)
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

int PlaylistModel::findTrack(const KUrl &url) const
{
    for (int i = 0; i < m_tracks.count(); ++i)
    {
        if (m_tracks.at(i) == url)
        {
            return i;
        }
    }

    return 0;
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
    return (index.isValid()?0:9);
}

int PlaylistModel::rowCount(const QModelIndex &index) const
{
    return (index.isValid()?0:m_tracks.count());
}

MetaDataKey PlaylistModel::translateColumn(int column) const
{
    switch (column)
    {
        case 1:
            return ArtistKey;
        case 2:
            return TitleKey;
        case 3:
            return AlbumKey;
        case 4:
            return TrackNumberKey;
        case 5:
            return GenreKey;
        case 6:
            return DescriptionKey;
        case 7:
            return DateKey;
        default:
            return InvalidKey;
    }

    return InvalidKey;
}

bool PlaylistModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_tracks.count() || role != Qt::EditRole || index.column() < 1 || index.column() > 7)
    {
        return false;
    }

    MetaDataManager::setMetaData(m_tracks.at(index.row()), translateColumn(index.column()), value.toString());

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

    const int end = (row + count);

    beginInsertRows(index, row, (end - 1));

    for (int i = 0; i < count; ++i)
    {
        m_tracks.insert((row + i), KUrl());
    }

    endInsertRows();

    if (row <= m_currentTrack)
    {
        setCurrentTrack(qMin(end, (m_tracks.count() - 1)));
    }

    emit needsSaving();

    return true;
}

bool PlaylistModel::removeRows(int row, int count, const QModelIndex &index)
{
    if (!index.isValid() || row < 0 || row >= m_tracks.count())
    {
        return false;
    }

    KUrl::List removedTracks;
    const int end = (row + count);

    beginRemoveRows(index, row, (end - 1));

    for (int i = 0; i < (end - row); ++i)
    {
        removedTracks.append(m_tracks.at(row));

        m_tracks.removeAt(row);
    }

    endRemoveRows();

    m_manager->removeTracks(removedTracks);

    if (row < m_currentTrack)
    {
        setCurrentTrack((m_currentTrack - count), ((m_currentTrack >= row && m_currentTrack <= end && (m_manager->state() != StoppedState && isCurrent()))?StopReaction:NoReaction));
    }
    else
    {
        setCurrentTrack(m_currentTrack);
    }

    return true;
}

bool PlaylistModel::isReadOnly() const
{
    return (m_source != LocalSource);
}

bool PlaylistModel::isCurrent() const
{
    return (m_manager->playlists().value(m_manager->currentPlaylist()) == this);
}

}
