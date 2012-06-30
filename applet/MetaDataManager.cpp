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

#include "MetaDataManager.h"

#include <QtCore/QFileInfo>
#include <QtCore/QTimerEvent>

#include <KMimeType>

namespace MiniPlayer
{

QQueue<QPair<KUrl, int> > MetaDataManager::m_queue;
QMap<KUrl, Track> MetaDataManager::m_tracks;
MetaDataManager* MetaDataManager::m_instance = NULL;

MetaDataManager::MetaDataManager(QObject *parent) : QObject(parent),
    m_mediaObject(new Phonon::MediaObject(this)),
    m_resolveMedia(0),
    m_attempts(0)
{
    m_keys << qMakePair(ArtistKey, Phonon::ArtistMetaData)
    << qMakePair(TitleKey, Phonon::TitleMetaData)
    << qMakePair(AlbumKey, Phonon::AlbumMetaData)
    << qMakePair(DateKey, Phonon::DateMetaData)
    << qMakePair(GenreKey, Phonon::GenreMetaData)
    << qMakePair(DescriptionKey, Phonon::DescriptionMetaData)
    << qMakePair(TrackNumberKey, Phonon::TracknumberMetaData);
}

void MetaDataManager::createInstance(QObject *parent)
{
    m_instance = new MetaDataManager(parent);
}

void MetaDataManager::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event)

    killTimer(event->timerId());

    resolveMetaData();
}

void MetaDataManager::resolveMetaData()
{
    QPair<KUrl, int> url;

    killTimer(m_resolveMedia);

    if (m_mediaObject->currentSource().url().isValid())
    {
        Track track;
        track.duration = m_mediaObject->totalTime();

        for (int i = 0; i < m_keys.count(); ++i)
        {
            const QStringList values = m_mediaObject->metaData(m_keys.at(i).second);

            if (!values.isEmpty() && !values.first().isEmpty())
            {
                track.keys[m_keys.at(i).first] = values.first();
            }
        }

        m_mediaObject->stop();

        if (track.keys.contains(TitleKey) && !track.keys[TitleKey].isEmpty())
        {
            setMetaData(m_mediaObject->currentSource().url(), track);
        }
        else if (m_attempts < 5)
        {
            ++m_attempts;

            m_queue.insert(m_attempts, qMakePair(KUrl(m_mediaObject->currentSource().url()), m_attempts));
        }
        else
        {
            const QString path = urlToTitle(KUrl(m_mediaObject->currentSource().url()));
            QRegExp trackExpression("^(?:\\s*(.+)\\s*-)?\\s*(.+)\\s*-\\s*(.+)\\s*$");

            if (trackExpression.exactMatch(path))
            {
                if (!trackExpression.cap(1).isEmpty())
                {
                    track.keys[TrackNumberKey] = trackExpression.cap(1).simplified();
                }

                track.keys[ArtistKey] = trackExpression.cap(2).simplified();
                track.keys[TitleKey] = trackExpression.cap(3).simplified();
            }
            else
            {
                track.keys[TitleKey] = path.simplified();
            }

            setMetaData(m_mediaObject->currentSource().url(), track);
        }
    }

    m_mediaObject->deleteLater();
    m_mediaObject = new Phonon::MediaObject(this);

    while (!m_queue.isEmpty())
    {
        url = m_queue.dequeue();

        if (!url.first.isValid() || !url.first.isLocalFile() || (m_tracks.contains(url.first) && m_tracks[url.first].duration > 0))
        {
            continue;
        }

        m_attempts = url.second;

        m_mediaObject->setCurrentSource(Phonon::MediaSource(url.first));
        m_mediaObject->play();

        m_resolveMedia = startTimer(200 + (m_attempts * 100));

        return;
    }

    m_mediaObject->setCurrentSource(Phonon::MediaSource());
}

void MetaDataManager::resolveTracks(const KUrl::List &urls)
{
    m_instance->addTracks(urls);
}

void MetaDataManager::addTracks(const KUrl::List &urls)
{
    for (int i = (urls.count() - 1); i >= 0 ; --i)
    {
        m_queue.prepend(qMakePair(urls.value(i), 0));
    }

    if (!m_mediaObject->currentSource().url().isValid())
    {
        resolveMetaData();
    }
}

void MetaDataManager::setDuration(const KUrl &url, qint64 duration)
{
    if (duration < 1)
    {
        return;
    }

    if (!m_tracks.contains(url))
    {
        m_tracks[url] = Track();
    }

    m_tracks[url].duration = duration;

    m_instance->setMetaData(url, m_tracks[url], true);
}

void MetaDataManager::setMetaData(const KUrl &url, MetaDataKey key, const QString &value)
{
    if (value.isEmpty())
    {
        return;
    }

    if (!m_tracks.contains(url))
    {
        m_tracks[url] = Track();
        m_tracks[url].duration = -1;
    }

    m_tracks[url].keys[key] = value;

    m_instance->setMetaData(url, m_tracks[url], true);
}

void MetaDataManager::setMetaData(const KUrl &url, const Track &track)
{
    m_instance->setMetaData(url, track, !isAvailable(url));
}

void MetaDataManager::setMetaData(const KUrl &url, const Track &track, bool notify)
{
    if ((track.keys.isEmpty() && track.duration < 1) || !url.isValid())
    {
        return;
    }

    m_tracks[url] = track;

    if (notify)
    {
        emit urlChanged(url);
    }
}

void MetaDataManager::removeMetaData(const KUrl &url)
{
    QList<QPair<KUrl, int> >::iterator i;

    for (i = m_queue.begin(); i != m_queue.end(); ++i)
    {
        if ((*i).first == url)
        {
            m_queue.removeAll(*i);
        }
    }

    m_tracks.remove(url);
}

MetaDataManager* MetaDataManager::instance()
{
    return m_instance;
}

KUrl::List MetaDataManager::tracks()
{
    return m_tracks.keys();
}

QVariantMap MetaDataManager::metaData(const KUrl &url)
{
    QVariantMap trackData;
    trackData["title"] = metaData(url, TitleKey, false);
    trackData["artist"] = metaData(url, ArtistKey, false);
    trackData["album"] = metaData(url, AlbumKey, false);
    trackData["date"] = metaData(url, DateKey, false);
    trackData["genre"] = metaData(url, GenreKey, false);
    trackData["description"] = metaData(url, DescriptionKey, false);
    trackData["tracknumber"] = metaData(url, TrackNumberKey, false);
    trackData["time"] = duration(url);
    trackData["location"] = url.pathOrUrl();

    return trackData;
}

QString MetaDataManager::metaData(const KUrl &url, MetaDataKey key, bool substitute)
{
    const bool exists = (m_tracks.contains(url) && m_tracks[url].keys.contains(key) && !m_tracks[url].keys[key].isEmpty());

    if (exists)
    {
        return m_tracks[url].keys[key];
    }

    if (!substitute)
    {
        return QString();
    }

    switch (key)
    {
        case TitleKey:
            return urlToTitle(url);
        case ArtistKey:
            return i18n("Unknown artist");
        default:
            return QString();
    }

    return QString();
}

QString MetaDataManager::timeToString(qint64 time)
{
    if (time < 1)
    {
        return QString("-:--:--");
    }

    QString string;
    int seconds = (time / 1000);
    int minutes = (seconds / 60);
    int hours = (minutes / 60);

    string.append(QString::number(hours));
    string.append(':');

    minutes = (minutes - (hours * 60));

    if (minutes < 10)
    {
        string.append('0');
    }

    string.append(QString::number(minutes));
    string.append(':');

    seconds = (seconds - (hours * 3600) - (minutes * 60));

    if (seconds < 10)
    {
        string.append('0');
    }

    string.append(QString::number(seconds));

    return string;
}

QString MetaDataManager::urlToTitle(const KUrl &url)
{
    QString title = QFileInfo(url.pathOrUrl()).completeBaseName();
    title = title.replace(QString("%20"), QChar(' '));
    title = title.replace(QChar('_'), QChar(' '));

    return title;
}

KIcon MetaDataManager::icon(const KUrl &url)
{
    if (url.isValid())
    {
        return KIcon(KMimeType::iconNameForUrl(url));
    }
    else
    {
        return KIcon("application-x-zerosize");
    }
}

qint64 MetaDataManager::duration(const KUrl &url)
{
    if (m_tracks.contains(url))
    {
        return m_tracks[url].duration;
    }

    return -1;
}

bool MetaDataManager::isAvailable(const KUrl &url, bool complete)
{
    return (m_tracks.contains(url) && !metaData(url, TitleKey, false).isEmpty() && (!complete || (!metaData(url, TitleKey, false).isEmpty() && !m_tracks[url].duration > 0)));
}

}
