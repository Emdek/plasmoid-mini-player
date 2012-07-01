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

#include "DBusPlayerAdaptor.h"
#include "DBusInterface.h"
#include "Player.h"
#include "PlaylistModel.h"
#include "MetaDataManager.h"

#include <QtCore/QCryptographicHash>

namespace MiniPlayer
{

DBusPlayerAdaptor::DBusPlayerAdaptor(QObject *parent, Player *player) : QDBusAbstractAdaptor(parent),
    m_player(player)
{
    m_properties["PlaybackStatus"] = PlaybackStatus();
    m_properties["LoopStatus"] = LoopStatus();
    m_properties["Shuffle"] = Shuffle();
    m_properties["Metadata"] = Metadata();
    m_properties["Volume"] = Volume();
    m_properties["CanGoNext"] = CanGoNext();
    m_properties["CanGoPrevious"] = CanGoPrevious();
    m_properties["CanPlay"] = CanPlay();
    m_properties["CanPause"] = CanPause();
    m_properties["CanSeek"] = CanSeek();

    connect(m_player, SIGNAL(stateChanged(PlayerState)), this, SLOT(updateProperties()));
    connect(m_player, SIGNAL(playbackModeChanged(PlaybackMode)), this, SLOT(updateProperties()));
    connect(m_player, SIGNAL(volumeChanged(int)), this, SLOT(updateProperties()));
    connect(m_player, SIGNAL(trackAdded(int)), this, SLOT(updateProperties()));
    connect(m_player, SIGNAL(trackRemoved(int)), this, SLOT(updateProperties()));
    connect(m_player, SIGNAL(playlistChanged()), this, SLOT(updateProperties()));
    connect(m_player, SIGNAL(seekableChanged(bool)), this, SLOT(updateProperties()));
    connect(m_player, SIGNAL(metaDataChanged()), this, SLOT(emitMetaDataChanged()));
    connect(m_player, SIGNAL(currentTrackChanged()), this, SLOT(emitMetaDataChanged()));
    connect(m_player, SIGNAL(positionChanged(qint64)), this, SLOT(emitSeeked(qint64)));
}

void DBusPlayerAdaptor::Next() const
{
    m_player->playNext();
}

void DBusPlayerAdaptor::Previous() const
{
    m_player->playPrevious();
}

void DBusPlayerAdaptor::Pause() const
{
    m_player->pause();
}

void DBusPlayerAdaptor::PlayPause() const
{
    m_player->playPause();
}

void DBusPlayerAdaptor::Stop() const
{
    m_player->stop();
}

void DBusPlayerAdaptor::Play() const
{
    m_player->play();
}

void DBusPlayerAdaptor::Seek(qint64 offset) const
{
    m_player->setPosition(m_player->position() + (offset / 1000));
}

void DBusPlayerAdaptor::SetPosition(const QDBusObjectPath &trackId, qint64 position) const
{
    if (m_player->playlist() && QString("/track_%1").arg(m_player->playlist()->currentTrack()) == trackId.path())
    {
        m_player->setPosition(position / 1000);
    }
}

void DBusPlayerAdaptor::OpenUri(QString uri) const
{
    if (m_player->playlist())
    {
        m_player->playlist()->addTracks(KUrl::List(uri), -1, PlayReaction);
    }
}

void DBusPlayerAdaptor::setLoopStatus(const QString &loopStatus) const
{
    if (loopStatus == "Playlist")
    {
        m_player->setPlaybackMode(LoopPlaylistMode);
    }
    else if (loopStatus == "Track")
    {
        m_player->setPlaybackMode(LoopTrackMode);
    }
    else
    {
        m_player->setPlaybackMode(SequentialMode);
    }
}

void DBusPlayerAdaptor::setRate(double rate) const
{
    Q_UNUSED(rate)
}

void DBusPlayerAdaptor::setShuffle(bool shuffle) const
{
    if (shuffle)
    {
        m_player->setPlaybackMode(RandomMode);
    }
    else
    {
        m_player->setPlaybackMode(SequentialMode);
    }
}

void DBusPlayerAdaptor::setVolume(double volume) const
{
    m_player->setVolume(volume * 100);
}

void DBusPlayerAdaptor::updateProperties()
{
    QVariantMap properties;

    if (m_properties["PlaybackStatus"] != PlaybackStatus())
    {
        properties["PlaybackStatus"] = PlaybackStatus();
    }

    if (m_properties["LoopStatus"] != LoopStatus())
    {
        properties["LoopStatus"] = LoopStatus();
    }

    if (m_properties["Shuffle"] != Shuffle())
    {
        properties["Shuffle"] = Shuffle();
    }

    if (m_properties["Volume"] != Volume())
    {
        properties["Volume"] = Volume();
    }

    if (m_properties["CanGoNext"] != CanGoNext())
    {
        properties["CanGoNext"] = CanGoNext();
    }

    if (m_properties["CanGoPrevious"] != CanGoPrevious())
    {
        properties["CanGoPrevious"] = CanGoPrevious();
    }

    if (m_properties["CanPlay"] != CanPlay())
    {
        properties["CanPlay"] = CanPlay();
    }

    if (m_properties["CanPause"] != CanPause())
    {
        properties["CanPause"] = CanPause();
    }

    if (m_properties["CanSeek"] != CanSeek())
    {
        properties["CanSeek"] = CanSeek();
    }

    if (properties.isEmpty())
    {
        return;
    }

    QVariantMap::iterator iterator;

    for (iterator = properties.begin(); iterator != properties.end(); ++iterator)
    {
        m_properties[iterator.key()] = iterator.value();
    }

    DBusInterface::updateProperties("org.mpris.MediaPlayer2.Player", properties);
}

void DBusPlayerAdaptor::emitMetaDataChanged()
{
    if (m_properties["Metadata"] == Metadata())
    {
        return;
    }

    m_properties["Metadata"] = Metadata();

    QVariantMap properties;
    properties["Metadata"] = m_properties["Metadata"];

    DBusInterface::updateProperties("org.mpris.MediaPlayer2.Player", properties);
}

void DBusPlayerAdaptor::emitSeeked(qint64 position)
{
    Q_EMIT Seeked(position * 1000);
}

QVariantMap DBusPlayerAdaptor::Metadata() const
{
    QVariantMap metaData;
    const KUrl url(m_player->url());

    if (!url.isValid() || m_player->state() == StoppedState || m_player->state() == ErrorState || !m_player->playlist())
    {
        return metaData;
    }

    metaData["mpris:trackid"] = QString("/track_%1").arg(m_player->playlist()->currentTrack());
    metaData["mpris:length"] = (m_player->duration() * 1000);
    metaData["xesam:url"] = url.pathOrUrl();
    metaData["xesam:title"] = m_player->metaData(TitleKey);
    metaData["xesam:artist"] = QStringList(m_player->metaData(ArtistKey));
    metaData["xesam:album"] = m_player->metaData(AlbumKey);
    metaData["xesam:genre"] = QStringList(m_player->metaData(GenreKey));
    metaData["xesam:comment"] = QStringList(m_player->metaData(DescriptionKey));
    metaData["xesam:trackNumber"] = m_player->metaData(TrackNumberKey);

    return metaData;
}

QString DBusPlayerAdaptor::PlaybackStatus() const
{
    switch (m_player->state())
    {
        case PlayingState:
            return "Playing";
        case PausedState:
            return "Paused";
        default:
            return "Stopped";
    }
}

QString DBusPlayerAdaptor::LoopStatus() const
{
    if (m_player->playbackMode() == SequentialMode)
    {
        return "None";
    }
    else if (m_player->playbackMode() == LoopTrackMode || m_player->playbackMode() == CurrentTrackOnceMode)
    {
        return "Track";
    }

    return "Playlist";
}

qint64 DBusPlayerAdaptor::Position() const
{
    return (m_player->position() * 1000);
}

double DBusPlayerAdaptor::Rate() const
{
    return 1.0;
}

double DBusPlayerAdaptor::Volume() const
{
    return ((double) m_player->volume() / 100);
}

double DBusPlayerAdaptor::MinimumRate() const
{
    return 1.0;
}

double DBusPlayerAdaptor::MaximumRate() const
{
    return 1.0;
}

bool DBusPlayerAdaptor::Shuffle() const
{
    return (m_player->playbackMode() == RandomMode);
}

bool DBusPlayerAdaptor::CanGoNext() const
{
    return (m_player->playlist() && m_player->playlist()->trackCount() > 1);
}

bool DBusPlayerAdaptor::CanGoPrevious() const
{
    return (m_player->playlist() && m_player->playlist()->trackCount() > 1);
}

bool DBusPlayerAdaptor::CanPause() const
{
    return (m_player->playlist() && m_player->playlist()->trackCount() > 0 && m_player->state() != ErrorState);
}

bool DBusPlayerAdaptor::CanPlay() const
{
    return (m_player->playlist() && m_player->playlist()->trackCount() > 0);
}

bool DBusPlayerAdaptor::CanSeek() const
{
    return m_player->isSeekable();
}

bool DBusPlayerAdaptor::CanControl() const
{
    return true;
}

}
