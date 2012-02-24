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
QHash<KUrl, Track> MetaDataManager::m_tracks;
MetaDataManager* MetaDataManager::m_instance = NULL;

MetaDataManager::MetaDataManager(QObject *parent) : QObject(parent),
    m_mediaObject(new Phonon::MediaObject(this)),
    m_resolveMedia(0),
    m_attempts(0)
{
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
        const QStringList titles = m_mediaObject->metaData(Phonon::TitleMetaData);
        const QStringList artists = m_mediaObject->metaData(Phonon::ArtistMetaData);
        Track track;
        track.title = (titles.isEmpty()?QString():titles.first());
        track.artist = (artists.isEmpty()?QString():artists.first());
        track.duration = m_mediaObject->totalTime();

        m_mediaObject->stop();

        if (!track.title.isEmpty() || track.duration > 0)
        {
            setMetaData(m_mediaObject->currentSource().url(), track);
        }
        else if (m_attempts < 6)
        {
            ++m_attempts;

            m_queue.insert(m_attempts, qMakePair(KUrl(m_mediaObject->currentSource().url()), m_attempts));
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

void MetaDataManager::setTitle(const KUrl &url, const QString &title)
{
    if (title.isEmpty())
    {
        return;
    }

    if (!m_tracks.contains(url))
    {
        m_tracks[url] = Track();
        m_tracks[url].duration = -1;
    }

    m_tracks[url].title = title;

    m_instance->setMetaData(url, m_tracks[url], true);
}

void MetaDataManager::setArtist(const KUrl &url, const QString &artist)
{
    if (artist.isEmpty())
    {
        return;
    }

    if (!m_tracks.contains(url))
    {
        m_tracks[url] = Track();
        m_tracks[url].duration = -1;
    }

    m_tracks[url].artist = artist;

    m_instance->setMetaData(url, m_tracks[url], true);
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

void MetaDataManager::setMetaData(const KUrl &url, const Track &track)
{
    m_instance->setMetaData(url, track, !isAvailable(url));
}

void MetaDataManager::setMetaData(const KUrl &url, const Track &track, bool notify)
{
    if ((track.title.isEmpty() && track.duration < 1) || !url.isValid())
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

QVariantMap MetaDataManager::metaData(const KUrl &url)
{
    Phonon::MediaObject mediaObject;
    mediaObject.setCurrentSource(Phonon::MediaSource(url));

    QVariantMap metaData;
    QMultiMap<QString, QString> stringMap = mediaObject.metaData();
    QMultiMap<QString, QString>::const_iterator i = stringMap.constBegin();

    while (i != stringMap.constEnd())
    {
        bool number = false;
        int value = i.value().toInt(&number);

        if (number && (i.key().toLower() != "tracknumber"))
        {
            metaData[i.key().toLower()] = value;
        }
        else
        {
            metaData[i.key().toLower()] = QVariant(i.value());
        }

        ++i;
    }

    metaData["time"] = ((duration(url) > 0)?duration(url):((mediaObject.totalTime() > 0)?(mediaObject.totalTime() / 1000):-1));
    metaData["location"] = url.pathOrUrl();

    return metaData;
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

KUrl::List MetaDataManager::tracks()
{
    return m_tracks.keys();
}

QString MetaDataManager::title(const KUrl &url, bool allowSubstitute )
{
    QString title;

    if (m_tracks.contains(url) && !m_tracks[url].title.isEmpty())
    {
        title = m_tracks[url].title;
    }
    else if (allowSubstitute)
    {
        title = urlToTitle(url);
    }

    return title;
}

QString MetaDataManager::artist(const KUrl &url, bool allowSubstitute )
{
    QString artist;

    if (m_tracks.contains(url) && !m_tracks[url].artist.isEmpty())
    {
        artist = m_tracks[url].artist;
    }
    else if (allowSubstitute)
    {
        artist = i18n("Unknown artist");
    }

    return artist;
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
    return (m_tracks.contains(url) && !m_tracks[url].title.isEmpty() && (!complete || (!m_tracks[url].title.isEmpty() && !m_tracks[url].duration > 0)));
}

}
