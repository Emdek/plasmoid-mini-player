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

#include <QGlib/Connect>

#include <QGst/Bus>
#include <QGst/Event>
#include <QGst/Query>
#include <QGst/TagList>
#include <QGst/ElementFactory>

#include <KMimeType>

namespace MiniPlayer
{

QQueue<QPair<KUrl, int> > MetaDataManager::m_queue;
QMap<KUrl, Track> MetaDataManager::m_tracks;
MetaDataManager* MetaDataManager::m_instance = NULL;

MetaDataManager::MetaDataManager(QObject *parent) : QObject(parent),
    m_scheduleNextTimer(0),
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

    scheduleNext();
}

void MetaDataManager::handleBusMessage(const QGst::MessagePtr &message)
{
    switch (message->type())
    {
        case QGst::MessageEos:
            scheduleNext();
        case QGst::MessageError:
            scheduleNext();

            break;
        case QGst::MessageTag:
            if (m_pipeline)
            {
                const QGst::TagList tags = message.staticCast<QGst::TagMessage>()->taglist();

                if (tags.isEmpty())
                {
                    break;
                }

                Track track;

                if (!tags.artist().isEmpty())
                {
                    track.keys[ArtistKey] = tags.artist();
                }

                if (!tags.title().isEmpty())
                {
                    track.keys[TitleKey] = tags.title();
                }

                if (!tags.tagValue("ALBUM").toString().isEmpty())
                {
                    track.keys[AlbumKey] = tags.tagValue("ALBUM").toString();
                }

                if (tags.trackNumber() > 0)
                {
                    track.keys[TrackNumberKey] = QString::number(tags.trackNumber());
                }

                if (!tags.genre().isEmpty())
                {
                    track.keys[GenreKey] = tags.genre();
                }

                if (!tags.description().isEmpty())
                {
                    track.keys[DescriptionKey] = tags.description();
                }

                if (tags.date().isValid())
                {
                    track.keys[DateKey] = tags.date().toString();
                }

                if (!track.keys.isEmpty())
                {
                    track.duration = duration(m_url);

                    setMetaData(m_url, track);

                    if (track.duration >= 0)
                    {
                        scheduleNext();
                    }
                }
            }

            break;
        case QGst::MessageDuration:
            if (m_pipeline)
            {
                QGst::DurationQueryPtr query = QGst::DurationQuery::create(QGst::FormatTime);

                m_pipeline->query(query);

                setDuration(m_url, (query->duration() / 1000000));

                if (isAvailable(m_url))
                {
                    scheduleNext();
                }
            }

            break;
        default:
            break;
    }
}

void MetaDataManager::scheduleNext()
{
    killTimer(m_scheduleNextTimer);

    if (m_url.isValid() && m_pipeline)
    {
        m_pipeline->setState(QGst::StateNull);

        if (m_attempts < 5)
        {
            ++m_attempts;

            m_queue.append(qMakePair(m_url, m_attempts));
        }
        else
        {
            const QString path = urlToTitle(m_url);
            QRegExp trackExpression("^(?:\\s*(.+)\\s*-)?\\s*(.+)\\s*-\\s*(.+)\\s*$");
            Track track;

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

            setMetaData(m_url, track);
        }

        m_url = KUrl();
    }

    if (!m_pipeline)
    {
        m_pipeline = QGst::ElementFactory::make("playbin2").dynamicCast<QGst::Pipeline>();

        if (m_pipeline)
        {
            QGst::BusPtr bus = m_pipeline->bus();
            bus->addSignalWatch();

            m_pipeline->setProperty("mute", true);
            m_pipeline->setProperty("audio-sink", QGst::ElementFactory::make("fakesink"));
            m_pipeline->setProperty("video-sink", QGst::ElementFactory::make("fakesink"));

            QGlib::connect(bus, "message", this, &MetaDataManager::handleBusMessage);
        }
    }

    if (!m_pipeline)
    {
        return;
    }

    while (!m_queue.isEmpty())
    {
        QPair<KUrl, int> pair = m_queue.dequeue();

        if (!pair.first.isValid() || !pair.first.isLocalFile() || isAvailable(pair.first))
        {
            continue;
        }

        m_url = pair.first;
        m_attempts = pair.second;

        m_pipeline->setProperty("uri", m_url.url());
        m_pipeline->setState(QGst::StatePlaying);

        m_scheduleNextTimer = startTimer(200 + (m_attempts * 100));

        return;
    }
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

    if (!m_url.isValid())
    {
        scheduleNext();
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
        Q_EMIT urlChanged(url);
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

bool MetaDataManager::isAvailable(const KUrl &url)
{
    return (m_tracks.contains(url) && !m_tracks[url].keys.isEmpty());
}

}
