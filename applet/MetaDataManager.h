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

#ifndef MINIPLAYERMETADATAMANAGER_HEADER
#define MINIPLAYERMETADATAMANAGER_HEADER

#include <QtCore/QQueue>

#include <KUrl>
#include <KIcon>

#include <Phonon/MediaObject>

namespace MiniPlayer
{

struct Track
{
    QString title;
    QString artist;
    qint64 duration;
};

class MetaDataManager : public QObject
{
    Q_OBJECT

    public:
        static void createInstance(QObject *parent = NULL);
        static void addTracks(const KUrl::List &urls);
        static void setMetaData(const KUrl &url, const Track &track);
        static void setMetaData(const KUrl &url, const QString &title, const QString &artist, qint64 duration);
        static MetaDataManager* instance();
        static QString timeToString(qint64 time);
        static QVariantMap metaData(const KUrl &url);
        static KUrl::List tracks();
        static QString title(const KUrl &url);
        static QString artist(const KUrl &url);
        static KIcon icon(const KUrl &url);
        static qint64 duration(const KUrl &url);
        static bool isAvailable(const KUrl &url);

    protected:
        MetaDataManager(QObject *parent);

        void timerEvent(QTimerEvent *event);
        void resolveMetaData();
        void addUrls(const KUrl::List &urls);
        void setMetaData(const KUrl &url, const Track &track, bool notify);

    private:
        Phonon::MediaObject *m_mediaObject;
        QQueue<QPair<KUrl, int> > m_queue;
        int m_resolveMedia;
        int m_attempts;

        static QHash<KUrl, Track> m_tracks;
        static MetaDataManager *m_instance;

    signals:
        void urlChanged(KUrl url);
};

}

#endif
