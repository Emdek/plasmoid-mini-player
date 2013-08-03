/***********************************************************************************
* Mini Player: Advanced media player for Plasma.
* Copyright (C) 2008 - 2013 Michal Dutkiewicz aka Emdek <emdeck@gmail.com>
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

#include "PlaylistReader.h"
#include "MetaDataManager.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QRegExp>
#include <QtCore/QFileInfo>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamAttributes>

#include <KLocale>
#include <KMimeType>
#include <KMessageBox>

namespace MiniPlayer
{

PlaylistReader::PlaylistReader(QObject *parent, const KUrl::List &urls, int index, PlayerReaction reaction) : QObject(parent),
    m_reaction(reaction),
    m_imports(0),
    m_index(index)
{
    connect(this, SIGNAL(processedTracks(KUrl::List,int,PlayerReaction)), parent, SLOT(processedTracks(KUrl::List,int,PlayerReaction)));

    addUrls(urls);
}

void PlaylistReader::addUrls(const KUrl::List &items, int level)
{
    ++m_imports;

    foreach (const KUrl &url, items)
    {
        PlaylistFormat format = InvalidFormat;

        if (url.isLocalFile())
        {
            KMimeType::Ptr mimeType = KMimeType::findByUrl(url);

            if (mimeType->is("inode/directory"))
            {
                readDirectory(url, level);

                continue;
            }
            else if (mimeType->is("audio/x-scpls"))
            {
                format = PlsFormat;
            }
            else if (mimeType->is("audio/x-mpegurl"))
            {
                format = M3uFormat;
            }
            else if (mimeType->is("application/xspf+xml"))
            {
                format = XspfFormat;
            }
            else if (mimeType->is("audio/x-ms-asx"))
            {
                format = AsxFormat;
            }

            if (format != InvalidFormat)
            {
                importPlaylist(url, format);

                continue;
            }

            const QString mimeTypeName = mimeType.data()->name();

            if (mimeTypeName.indexOf("video/") == -1 && mimeTypeName.indexOf("audio/") == -1 && mimeTypeName != "application/ogg")
            {
                continue;
            }
        }
        else
        {
            if (url.pathOrUrl().endsWith(QString(".pls"), Qt::CaseInsensitive))
            {
                format = PlsFormat;
            }
            else if (url.pathOrUrl().endsWith(QString(".m3u"), Qt::CaseInsensitive))
            {
                format = M3uFormat;
            }
            else if (url.pathOrUrl().endsWith(QString(".xpsf"), Qt::CaseInsensitive))
            {
                format = XspfFormat;
            }
            else if (url.pathOrUrl().endsWith(QString(".asx"), Qt::CaseInsensitive))
            {
                format = AsxFormat;
            }

            if (format != InvalidFormat)
            {
                KIO::Job *job = KIO::get(url, KIO::NoReload, KIO::HideProgressInfo);

                connect(job, SIGNAL(data(KIO::Job*,const QByteArray&)), this, SLOT(importPlaylistData(KIO::Job*,const QByteArray&)));
                connect(job, SIGNAL(result(KJob*)), this, SLOT(importPlaylistResult(KJob*)));

                job->start();

                m_remotePlaylists[job] = qMakePair(format, QByteArray());

                continue;
            }
        }

        m_tracks.append(url);
    }

    --m_imports;

    if (!m_imports)
    {
        emit processedTracks(m_tracks, m_index, m_reaction);

        deleteLater();
    }
}

void PlaylistReader::importPlaylist(const KUrl &url, PlaylistFormat format)
{
    QFileInfo currentLocation(url.pathOrUrl());

    QDir::setCurrent(currentLocation.absolutePath());

    QFile data(url.pathOrUrl());

    if (!data.open(QFile::ReadOnly))
    {
        KMessageBox::error(NULL, i18n("Cannot open file for reading."));
    }

    if (format == XspfFormat)
    {
        readXspf(data.readAll());
    }
    else if (format == AsxFormat)
    {
        readAsx(data.readAll());
    }
    else
    {
        QTextStream stream(&data);

        if (format == PlsFormat)
        {
            readPls(stream);
        }
        else
        {
            readM3u(stream);
        }
    }

    data.close();
}

void PlaylistReader::importData(KIO::Job *job, const QByteArray &data)
{
    m_remotePlaylists[job].second.append(data);
}

void PlaylistReader::importResult(KJob *job)
{
    if (m_remotePlaylists[job].first == XspfFormat)
    {
        readXspf(m_remotePlaylists[job].second);
    }
    else if (m_remotePlaylists[job].first == AsxFormat)
    {
        readAsx(m_remotePlaylists[job].second);
    }
    else
    {
        QTextStream stream(m_remotePlaylists[job].second);

        if (m_remotePlaylists[job].first == PlsFormat)
        {
            readPls(stream);
        }
        else
        {
            readM3u(stream);
        }
    }

    m_remotePlaylists.remove(job);
}

void PlaylistReader::readPls(QTextStream &stream)
{
    QRegExp plsUrl("^File\\d+=");
    QRegExp plsTitle("^Title\\d+=");
    QRegExp plsDuration("^Length\\d+=");
    QString line;
    KUrl::List urls;
    KUrl url;
    Track track;
    bool addUrl = false;

    do
    {
        line = stream.readLine();

        if (line.isEmpty())
        {
            continue;
        }

        if (addUrl)
        {
            MetaDataManager::setMetaData(url, track);

            track.keys.clear();
            track.duration = -1;
        }

        addUrl = false;

        if (line.contains(plsUrl))
        {
            addUrl = true;

            line = line.remove(plsUrl);
        }
        else if (line.contains(plsTitle))
        {
            track.keys[TitleKey] = line.remove(plsTitle).trimmed();
        }
        else if (line.contains(plsDuration))
        {
            track.duration = line.remove(plsDuration).trimmed().toLongLong();
        }

        if (addUrl)
        {
            url = KUrl(line);

            if (!url.isValid())
            {
                continue;
            }

            if (!url.isLocalFile())
            {
                urls.append(url);

                continue;
            }

            QFileInfo location(line);
            location.makeAbsolute();

            if (location.exists())
            {
                urls.append(KUrl(location.filePath()));
            }
        }
    }
    while (!line.isNull());

    addUrls(urls);
}

void PlaylistReader::readM3u(QTextStream &stream)
{
    QRegExp m3uInformation("^#EXTINF:");
    QString line;
    KUrl::List urls;
    Track track;
    bool addUrl;

    do
    {
        line = stream.readLine();

        if (line.isEmpty())
        {
            continue;
        }

        addUrl = false;

        if (!line.startsWith('#'))
        {
            addUrl = true;
        }

        if (line.contains(m3uInformation))
        {
            const QStringList information = line.remove(m3uInformation).split(',');

            track.keys[TitleKey] = information.value(1);
            track.duration = information.value(0).toLongLong();
        }

        if (addUrl)
        {
            KUrl url(line);

            if (!url.isValid())
            {
                continue;
            }

            MetaDataManager::setMetaData(url, track);

            track.keys.clear();
            track.duration = -1;

            if (!url.isLocalFile())
            {
                urls.append(url);

                continue;
            }

            QFileInfo location(line);
            location.makeAbsolute();

            if (location.exists())
            {
                urls.append(KUrl(location.filePath()));
            }
        }
    }
    while (!line.isNull());

    addUrls(urls);
}

void PlaylistReader::readXspf(const QByteArray &data)
{
    QXmlStreamReader reader(data);
    KUrl::List urls;
    KUrl url;
    Track track;

    while (!reader.atEnd())
    {
        reader.readNext();

        if (reader.name().toString() == "track")
        {
            track.keys.clear();
        }

        if (reader.name().toString() == "title")
        {
            track.keys[TitleKey] = reader.text().toString();
        }

        if (reader.name().toString() == "creator")
        {
            track.keys[ArtistKey] = reader.text().toString();
        }

        if (reader.name().toString() == "annotation")
        {
            track.keys[DescriptionKey] = reader.text().toString();
        }

        if (reader.name().toString() == "album")
        {
            track.keys[AlbumKey] = reader.text().toString();
        }

        if (reader.name().toString() == "trackNum")
        {
            track.keys[TrackNumberKey] = reader.text().toString();
        }

        if (reader.name().toString() == "genre")
        {
            track.keys[GenreKey] = reader.text().toString();
        }

        if (reader.name().toString() == "date")
        {
            track.keys[DateKey] = reader.text().toString();
        }

        if (reader.name().toString() == "location")
        {
            url = KUrl(reader.text().toString());

            if (!url.isValid())
            {
                continue;
            }

            if (!url.isLocalFile())
            {
                MetaDataManager::setMetaData(url, track);

                urls.append(url);

                continue;
            }

            QFileInfo location(url.toLocalFile());
            location.makeAbsolute();

            if (location.exists())
            {
                url = KUrl(location.filePath());

                MetaDataManager::setMetaData(url, track);

                urls.append(url);
            }
        }
    }

    addUrls(urls);
}

void PlaylistReader::readAsx(const QByteArray &data)
{
    QXmlStreamReader reader(data);
    KUrl::List urls;
    KUrl url;
    Track track;

    while (!reader.atEnd())
    {
        reader.readNext();

        if (reader.name().toString() == "entry")
        {
            track.keys.clear();
        }

        if (reader.name().toString() == "title")
        {
            track.keys[TitleKey] = reader.text().toString();
        }

        if (reader.name().toString() == "author")
        {
            track.keys[ArtistKey] = reader.text().toString();
        }

        if (reader.name().toString() == "ref")
        {
            url = KUrl(reader.attributes().value("href").toString());

            if (!url.isValid())
            {
                continue;
            }

            if (!url.isLocalFile())
            {
                urls.append(url);

                MetaDataManager::setMetaData(url, track);

                continue;
            }

            QFileInfo location(url.toLocalFile());
            location.makeAbsolute();

            if (location.exists())
            {
                url = KUrl(location.filePath());

                MetaDataManager::setMetaData(url, track);

                urls.append(url);
            }
        }
    }

    addUrls(urls);
}

void PlaylistReader::readDirectory(const KUrl &url, int level)
{
    if (level > 9)
    {
        return;
    }

    const QStringList entries = QDir(url.toLocalFile()).entryList(QDir::Readable | QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    KUrl::List urls;

    for (int i = 0; i < entries.count(); ++i)
    {
        KUrl newUrl = url;
        newUrl.addPath(entries.at(i));

        urls.append(newUrl);
    }

    addUrls(urls, ++level);
}

}
