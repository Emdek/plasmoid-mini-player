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

#include "MetaDataManager.h"

#include <QtCore/QFileInfo>

#include <KMimeType>

namespace MiniPlayer
{

QHash<KUrl, QPair<QString, qint64> > MetaDataManager::m_tracks;
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

    resolveMetaData();

    killTimer(m_resolveMedia);
}

void MetaDataManager::resolveMetaData()
{
    QPair<KUrl, int> track;

    killTimer(m_resolveMedia);

    if (m_mediaObject->currentSource().type() != Phonon::MediaSource::Invalid)
    {
        const QStringList titles = m_mediaObject->metaData(Phonon::TitleMetaData);
        const QString title = (titles.isEmpty()?QString():titles.first());
        const qint64 duration = m_mediaObject->totalTime();

        m_mediaObject->stop();

        if ((title.isEmpty() || duration < 1) && m_attempts < 6)
        {
            ++m_attempts;

            m_queue.insert(m_attempts, qMakePair(KUrl(m_mediaObject->currentSource().url()), m_attempts));
        }
        else
        {
            setMetaData(m_mediaObject->currentSource().url(), title, duration);

            emit urlChanged(m_mediaObject->currentSource().url());
        }
    }

    m_mediaObject->deleteLater();
    m_mediaObject = new Phonon::MediaObject(this);

    connect(m_mediaObject, SIGNAL(totalTimeChanged(qint64)), this, SLOT(resolveMetaData()));

    while (!m_queue.isEmpty())
    {
        track = m_queue.dequeue();

        if (!track.first.isValid() || !track.first.isLocalFile() || (m_tracks.contains(track.first) && m_tracks[track.first].second > 0))
        {
            continue;
        }

        m_attempts = track.second;

        m_mediaObject->setCurrentSource(Phonon::MediaSource(track.first));
        m_mediaObject->play();

        m_resolveMedia = startTimer(200 + (m_attempts * 100));

        return;
    }

    m_mediaObject->setCurrentSource(Phonon::MediaSource());
}

void MetaDataManager::addTracks(const KUrl::List &urls)
{
    m_instance->addUrls(urls);
}

void MetaDataManager::addUrls(const KUrl::List &urls)
{
    for (int i = 0; i < urls.count(); ++i)
    {
        m_queue.prepend(qMakePair(urls.at(i), 0));
    }

    if (!m_mediaObject->currentSource().url().isValid())
    {
        resolveMetaData();
    }
}

void MetaDataManager::setMetaData(const KUrl &url, const QString &title, qint64 duration)
{
    m_instance->setUrlMetaData(url, title, duration);
}

void MetaDataManager::setMetaData(const QHash<KUrl, QPair<QString, qint64> > &metaData)
{
    QHash<KUrl, QPair<QString, qint64> >::const_iterator iterator;

    for (iterator = metaData.begin(); iterator != metaData.end(); ++iterator)
    {
        m_instance->setUrlMetaData(iterator.key(), iterator.value().first, iterator.value().second);
    }
}

void MetaDataManager::setUrlMetaData(const KUrl &url, const QString &title, qint64 duration)
{
    if (title.isEmpty() && duration < 1)
    {
        return;
    }

    if (!isAvailable(url))
    {
        emit urlChanged(url);
    }

    if (duration < 1 && m_tracks.contains(url))
    {
        duration = m_tracks[url].second;
    }

    m_tracks[url] = qMakePair(title, duration);
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

    metaData["time"] = ((mediaObject.totalTime() > 0)?(mediaObject.totalTime() / 1000):-1);
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

KUrl::List MetaDataManager::tracks()
{
    return m_tracks.keys();
}

QString MetaDataManager::title(const KUrl &url)
{
    QString title;

    if (m_tracks.contains(url) && !m_tracks[url].first.isEmpty())
    {
        title = m_tracks[url].first;
    }
    else
    {
        title = QFileInfo(url.pathOrUrl()).completeBaseName().replace("%20", " ");
    }

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
        return m_tracks[url].second;
    }

    return -1;
}

bool MetaDataManager::isAvailable(const KUrl &url)
{
    return (!m_tracks.contains(url) || m_tracks[url].first.isEmpty() || m_tracks[url].second < 1);
}

}
