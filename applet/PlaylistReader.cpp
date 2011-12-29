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

#include "PlaylistReader.h"
#include "PlaylistModel.h"

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

PlaylistReader::PlaylistReader(PlaylistModel *parent, const KUrl::List &urls, int index, PlayerReaction reaction) : QObject(parent),
    m_reaction(reaction),
    m_imports(0),
    m_index(index)
{
    connect(this, SIGNAL(processedTracks(KUrl::List,QHash<KUrl,QPair<QString,qint64> >,int,PlayerReaction)), parent, SLOT(addTracks(KUrl::List,QHash<KUrl,QPair<QString,qint64> >,int,PlayerReaction)));

    addUrls(urls);
}

void PlaylistReader::addUrls(const KUrl::List &items, int level)
{
    ++m_imports;

    foreach (const KUrl &url, items)
    {
        PlaylistFormat type = None;

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
                type = PlsFormat;
            }
            else if (mimeType->is("audio/x-mpegurl"))
            {
                type = M3uFormat;
            }
            else if (mimeType->is("application/xspf+xml"))
            {
                type = XspfFormat;
            }
            else if (mimeType->is("audio/x-ms-asx"))
            {
                type = AsxFormat;
            }

            if (type != None)
            {
                importPlaylist(url, type);

                continue;
            }

            QString mimeTypeName = mimeType.data()->name();

            if (mimeTypeName.indexOf("video/") == -1 && mimeTypeName.indexOf("audio/") == -1 && mimeTypeName != "application/ogg")
            {
                continue;
            }
        }
        else
        {
            if (url.pathOrUrl().endsWith(QString(".pls"), Qt::CaseInsensitive))
            {
                type = PlsFormat;
            }
            else if (url.pathOrUrl().endsWith(QString(".m3u"), Qt::CaseInsensitive))
            {
                type = M3uFormat;
            }
            else if (url.pathOrUrl().endsWith(QString(".xpsf"), Qt::CaseInsensitive))
            {
                type = XspfFormat;
            }
            else if (url.pathOrUrl().endsWith(QString(".asx"), Qt::CaseInsensitive))
            {
                type = AsxFormat;
            }

            if (type != None)
            {
                KIO::Job *job = KIO::get(url, KIO::NoReload, KIO::HideProgressInfo);

                connect(job, SIGNAL(data(KIO::Job*,const QByteArray&)), this, SLOT(importPlaylistData(KIO::Job*,const QByteArray&)));
                connect(job, SIGNAL(result(KJob*)), this, SLOT(importPlaylistResult(KJob*)));

                job->start();

                m_remotePlaylistsData[job] = QByteArray();
                m_remotePlaylistsType[job] = type;

                continue;
            }
        }

        m_tracks.append(url);
    }

    --m_imports;

    if (!m_imports)
    {
        emit processedTracks(m_tracks, m_metaData, m_index, m_reaction);

        deleteLater();
    }
}

void PlaylistReader::importPlaylist(const KUrl &url, PlaylistFormat type)
{
    KUrl::List items;
    QFileInfo currentLocation(url.pathOrUrl());

    QDir::setCurrent(currentLocation.absolutePath());

    QFile data(url.pathOrUrl());

    if (data.open(QFile::ReadOnly))
    {
        if (type == XspfFormat)
        {
            QByteArray byteArray = data.readAll();

            readXspf(byteArray);
        }
        else if (type == AsxFormat)
        {
            QByteArray byteArray = data.readAll();

            readAsx(byteArray);
        }
        else
        {
            QTextStream stream(&data);

            if (type == PlsFormat)
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
    else
    {
        KMessageBox::error(NULL, i18n("Cannot open file for reading."));
    }

    addUrls(items);
}

void PlaylistReader::importData(KIO::Job* job, const QByteArray &data)
{
    m_remotePlaylistsData[job].append(data);
}

void PlaylistReader::importResult(KJob* job)
{
    if (m_remotePlaylistsType[job] == XspfFormat)
    {
        readXspf(m_remotePlaylistsData[job]);
    }
    else if (m_remotePlaylistsType[job] == AsxFormat)
    {
        readAsx(m_remotePlaylistsData[job]);
    }
    else
    {
        QTextStream stream(m_remotePlaylistsData[job]);

        if (m_remotePlaylistsType[job] == PlsFormat)
        {
            readPls(stream);
        }
        else
        {
            readM3u(stream);
        }
    }

    m_remotePlaylistsData.remove(job);
    m_remotePlaylistsType.remove(job);
}

void PlaylistReader::readM3u(QTextStream &stream)
{
    KUrl::List items;
    QRegExp m3uInformation("^#EXTINF:");
    QString line;
    QString title;
    qint64 duration = -1;
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
            QStringList information = line.remove(m3uInformation).split(',');

            title = information.value(1);
            duration = information.value(0).toLongLong();
        }

        if (addUrl)
        {
            KUrl url(line);

            if (!url.isValid())
            {
                continue;
            }

            if (!title.isEmpty() || duration > 0)
            {
                m_metaData[url] = qMakePair(title, duration);

                title = QString();
                duration = -1;
            }

            if (!url.isLocalFile())
            {
                items.append(url);

                continue;
            }

            QFileInfo location(line);
            location.makeAbsolute();

            if (location.exists())
            {
                items.append(KUrl(location.filePath()));
            }
        }
    }
    while (!line.isNull());

    addUrls(items);
}

void PlaylistReader::readPls(QTextStream &stream)
{
    KUrl::List items;
    KUrl url;
    QRegExp plsUrl("^File\\d+=");
    QRegExp plsTitle("^Title\\d+=");
    QRegExp plsDuration("^Length\\d+=");
    QString line;
    QString title;
    qint64 duration = -1;
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
            if (url.isValid() && (!title.isEmpty() || duration > 0))
            {
                m_metaData[url] = qMakePair(title, duration);
            }

            title = QString();
            duration = -1;
        }

        addUrl = false;

        if (line.contains(plsUrl))
        {
            addUrl = true;

            line = line.remove(plsUrl);
        }
        else if (line.contains(plsTitle))
        {
            title = line.remove(plsTitle).trimmed();
        }
        else if (line.contains(plsDuration))
        {
            duration = line.remove(plsDuration).trimmed().toLongLong();
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
                items.append(url);

                continue;
            }

            QFileInfo location(line);
            location.makeAbsolute();

            if (location.exists())
            {
                items.append(KUrl(location.filePath()));
            }
        }
    }
    while (!line.isNull());

    addUrls(items);
}

void PlaylistReader::readXspf(QByteArray &data)
{
    KUrl::List items;
    QXmlStreamReader reader(data);
    QString title;

    while (!reader.atEnd())
    {
        reader.readNext();

        if (reader.name().toString() == "track")
        {
            title = "";
        }

        if (reader.name().toString() == "title")
        {
            title = reader.text().toString();
        }

        if (reader.name().toString() == "location")
        {
            KUrl url(reader.text().toString());

            if (!url.isValid())
            {
                continue;
            }

            if (!url.isLocalFile())
            {
                items.append(url);

                continue;
            }

            QFileInfo location(url.toLocalFile());
            location.makeAbsolute();

            if (location.exists())
            {
                items.append(KUrl(location.filePath()));
            }
        }
    }

    addUrls(items);
}

void PlaylistReader::readAsx(QByteArray &data)
{
    KUrl::List items;
    QXmlStreamReader reader(data);
    QString title;

    while (!reader.atEnd())
    {
        reader.readNext();

        if (reader.name().toString() == "entry")
        {
            title = "";
        }

        if (reader.name().toString() == "title")
        {
            title = reader.text().toString();
        }

        if (reader.name().toString() == "ref")
        {
            QXmlStreamAttributes attributes = reader.attributes();

            KUrl url(attributes.value("href").toString());

            if (!url.isValid())
            {
                continue;
            }

            if (!url.isLocalFile())
            {
                items.append(url);

                continue;
            }

            QFileInfo location(url.toLocalFile());
            location.makeAbsolute();

            if (location.exists())
            {
                items.append(KUrl(location.filePath()));
            }
        }
    }

    addUrls(items);
}

void PlaylistReader::readDirectory(const KUrl &url, int level)
{
    if (level > 9)
    {
        return;
    }

    QStringList entries = QDir(url.toLocalFile()).entryList(QDir::Readable | QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
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
