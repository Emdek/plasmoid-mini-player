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

MetaDataManager::MetaDataManager(QObject *parent) : QObject(parent),
    m_mediaPlayer(new QMediaPlayer(this)),
    m_resolveMedia(0),
    m_attempts(0)
{
    m_mediaPlayer->setMuted(true);

    connect(m_mediaPlayer, SIGNAL(durationChanged(qint64)), this, SLOT(resolveMetaData()));
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

    if (m_mediaPlayer->media().canonicalUrl().isValid())
    {
        QString title = m_mediaPlayer->metaData(QtMultimediaKit::Title).toString();
        const qint64 duration = m_mediaPlayer->duration();

        m_mediaPlayer->stop();

        if ((title.isEmpty() || duration < 1) && m_attempts < 4)
        {
            ++m_attempts;

            m_queue.insert(m_attempts, qMakePair(KUrl(m_mediaPlayer->media().canonicalUrl()), m_attempts));
        }
        else
        {
            if (title.isEmpty())
            {
                title = QFileInfo(m_mediaPlayer->media().canonicalUrl().toString()).completeBaseName().replace("%20", " ");
            }

            setMetaData(m_mediaPlayer->media().canonicalUrl(), title, duration);

            emit urlChanged(m_mediaPlayer->media().canonicalUrl());
        }
    }

    while (!m_queue.isEmpty())
    {
        track = m_queue.dequeue();

        if (!track.first.isValid() || !track.first.isLocalFile() || (m_tracks.contains(track.first) && m_tracks[track.first].second > 0))
        {
            continue;
        }

        m_attempts = track.second;

        m_mediaPlayer->setMedia(QMediaContent(track.first));
        m_mediaPlayer->play();

        m_resolveMedia = startTimer(200);

        return;
    }

    m_mediaPlayer->setMedia(QMediaContent());
}

void MetaDataManager::addTracks(const KUrl::List &urls)
{
    for (int i = 0; i < urls.count(); ++i)
    {
        m_queue.append(qMakePair(urls.at(i), 0));
    }

    if (!m_mediaPlayer->media().canonicalUrl().isValid())
    {
        resolveMetaData();
    }
}

void MetaDataManager::setMetaData(const KUrl &url, const QString &title, qint64 duration)
{
    if (duration < 1 && m_tracks.contains(url))
    {
        duration = m_tracks[url].second;
    }

    m_tracks[url] = qMakePair(title, duration);
}

void MetaDataManager::setMetaData(const QHash<KUrl, QPair<QString, qint64> > &metaData)
{
    QHash<KUrl, QPair<QString, qint64> >::const_iterator iterator;

    for (iterator = metaData.begin(); iterator != metaData.end(); ++iterator)
    {
        if (!available(iterator.key()))
        {
            m_tracks[iterator.key()] = iterator.value();

            emit urlChanged(iterator.key());
        }
    }
}

QVariantMap MetaDataManager::metaData(const KUrl &url)
{
    QMediaPlayer *mediaPlayer = new QMediaPlayer(this);
    mediaPlayer->setMedia(QMediaContent(url));

    QVariantMap metaData;
    QStringList keys = mediaPlayer->availableExtendedMetaData();

    for (int i = 0; i < keys.count(); ++i)
    {
        bool number = false;
        int value = mediaPlayer->extendedMetaData(keys.at(i)).toInt(&number);

        if (number && (keys.at(i).toLower() != "tracknumber"))
        {
            metaData[keys.at(i).toLower()] = value;
        }
        else
        {
            metaData[keys.at(i).toLower()] = mediaPlayer->extendedMetaData(keys.at(i));
        }
    }

    metaData["time"] = ((mediaPlayer->duration() > 0)?(mediaPlayer->duration() / 1000):-1);
    metaData["location"] = url.pathOrUrl();

    mediaPlayer->deleteLater();

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

QString MetaDataManager::title(const KUrl &url) const
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

KIcon MetaDataManager::icon(const KUrl &url) const
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

qint64 MetaDataManager::duration(const KUrl &url) const
{
    if (m_tracks.contains(url))
    {
        return m_tracks[url].second;
    }

    return -1;
}

bool MetaDataManager::available(const KUrl &url) const
{
    return (!m_tracks.contains(url) || m_tracks[url].first.isEmpty() || m_tracks[url].second < 1);
}

}
