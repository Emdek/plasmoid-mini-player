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

#include "PlaylistWriter.h"
#include "PlaylistModel.h"
#include "MetaDataManager.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QXmlStreamWriter>

namespace MiniPlayer
{

PlaylistWriter::PlaylistWriter(QObject *parent, const QString &path, PlaylistModel *playlist, PlaylistFormat format) : QObject(parent),
    m_status(false)
{
    if (format == InvalidFormat)
    {
        if (path.endsWith(QString(".pls"), Qt::CaseInsensitive))
        {
            format = PlsFormat;
        }
        else if (path.endsWith(QString(".m3u"), Qt::CaseInsensitive))
        {
            format = M3uFormat;
        }
        else if (path.endsWith(QString(".xspf"), Qt::CaseInsensitive))
        {
            format = XspfFormat;
        }
        else if (path.endsWith(QString(".asx"), Qt::CaseInsensitive) || path.endsWith(QString(".asf"), Qt::CaseInsensitive))
        {
            format = AsxFormat;
        }
    }

    switch (format)
    {
        case PlsFormat:
            m_status = writePls(path, playlist->tracks());

            break;
        case M3uFormat:
            m_status = writeM3u(path, playlist->tracks());

            break;
        case XspfFormat:
            m_status = writeXspf(path, playlist);

            break;
        case AsxFormat:
            m_status = writeAsx(path, playlist);

            break;
        default:
            break;
    }
}

bool PlaylistWriter::writePls(const QString &path, const KUrl::List &tracks)
{
    QFile data(path);

    if (!data.open(QFile::WriteOnly))
    {
        return false;
    }

    QTextStream out(&data);
    out << "[playlist]\n";
    out << "NumberOfEntries=" << tracks.count() << "\n\n";

    for (int i = 0; i < tracks.count(); ++i)
    {
        const KUrl url = tracks.at(i);
        const QString title = MetaDataManager::metaData(url, TitleKey, false);
        const QString duration = ((MetaDataManager::duration(url) > 0)?QString::number(MetaDataManager::duration(url) / 1000):QString("-1"));
        const QString number = QString::number(i + 1);

        out << "File" << number << '=' << url.pathOrUrl() << '\n';
        out << "Title" << number << '=' << title << '\n';
        out << "Length" << number << '=' << duration << "\n\n";
    }

    out << "Version=2";

    data.close();

    return true;
}

bool PlaylistWriter::writeM3u(const QString &path, const KUrl::List &tracks)
{
    QFile data(path);

    if (!data.open(QFile::WriteOnly))
    {
        return false;
    }

    QTextStream out(&data);
    out << "#EXTM3U\n\n";

    for (int i = 0; i < tracks.count(); ++i)
    {
        const KUrl url = tracks.at(i);
        const QString title = MetaDataManager::metaData(url, TitleKey, false);
        const QString duration = ((MetaDataManager::duration(url) > 0)?QString::number(MetaDataManager::duration(url) / 1000):QString("-1"));

        out << "#EXTINF:" << duration << "," << title << "\n";
        out << url.pathOrUrl() << '\n';
    }

    data.close();

    return true;
}

bool PlaylistWriter::writeXspf(const QString &path, const PlaylistModel *playlist)
{
    QFile data(path);

    if (!data.open(QFile::WriteOnly) || !playlist)
    {
        return false;
    }

    QHash<MetaDataKey, QString> keys;
    keys[TitleKey] = "title";
    keys[ArtistKey] = "creator";
    keys[DescriptionKey] = "annotation";
    keys[AlbumKey] = "album";
    keys[TrackNumberKey] = "trackNum";

    QXmlStreamWriter stream(&data);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();
    stream.writeStartElement("playlist");
    stream.writeAttribute("xmlns", "http://xspf.org/ns/0/");
    stream.writeAttribute("version", "1");
    stream.writeTextElement("title", playlist->title());
    stream.writeStartElement("trackList");

    const KUrl::List tracks = playlist->tracks();

    for (int i = 0; i < tracks.count(); ++i)
    {
        const KUrl url = tracks.at(i);
        const qint64 duration = MetaDataManager::duration(url);

        stream.writeStartElement("track");
        stream.writeTextElement("location", url.pathOrUrl());

        QHash<MetaDataKey, QString>::iterator iterator;

        for (iterator = keys.begin(); iterator != keys.end(); ++iterator)
        {
            const QString value = MetaDataManager::metaData(url, iterator.key(), false);

            if (!value.isEmpty())
            {
                stream.writeTextElement(iterator.value(), value);
            }
        }

        if (duration >= 0)
        {
            stream.writeTextElement("duration", QString::number(duration));
        }

        stream.writeEndElement();
    }

    stream.writeEndElement();
    stream.writeEndElement();
    stream.writeEndDocument();

    data.close();

    return true;
}

bool PlaylistWriter::writeAsx(const QString &path, const PlaylistModel *playlist)
{
    QFile data(path);

    if (!data.open(QFile::WriteOnly) || !playlist)
    {
        return false;
    }

    QXmlStreamWriter stream(&data);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();
    stream.writeStartElement("asx");
    stream.writeAttribute("version", "3.0");
    stream.writeTextElement("title", playlist->title());

    const KUrl::List tracks = playlist->tracks();

    for (int i = 0; i < tracks.count(); ++i)
    {
        const KUrl url = tracks.at(i);
        const QString title = MetaDataManager::metaData(url, TitleKey, false);
        const QString author = MetaDataManager::metaData(url, ArtistKey, false);

        stream.writeStartElement("entry");

        if (!title.isEmpty())
        {
            stream.writeTextElement("title", title);
        }

        stream.writeStartElement("ref");
        stream.writeAttribute("href", url.pathOrUrl());
        stream.writeEndElement();

        if (!author.isEmpty())
        {
            stream.writeTextElement("author", author);
        }

        stream.writeEndElement();
    }

    stream.writeEndElement();
    stream.writeEndDocument();

    data.close();

    return true;
}

bool PlaylistWriter::status() const
{
    return m_status;
}

}
