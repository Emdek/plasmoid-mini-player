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
#include <QtCore/QObject>
#include <QtMultimediaKit/QMediaPlayer>

#include <KUrl>
#include <KIcon>

namespace MiniPlayer
{

class MetaDataManager : public QObject
{
    Q_OBJECT

    public:
        MetaDataManager(QObject *parent);

        static QString timeToString(qint64 time);
        QVariantMap metaData(const KUrl &url);
        QString title(const KUrl &url) const;
        KIcon icon(const KUrl &url) const;
        qint64 duration(const KUrl &url) const;
        bool available(const KUrl &url) const;

    public slots:
        void addTracks(const KUrl::List &urls);
        void setMetaData(const KUrl &url, const QString &title, qint64 duration);
        void setMetaData(const QHash<KUrl, QPair<QString, qint64> > &metaData);
        void resolveMetaData();

    protected:
        void timerEvent(QTimerEvent *event);

    private:
        QMediaPlayer *m_mediaPlayer;
        QHash<KUrl, QPair<QString, qint64> > m_tracks;
        QQueue<QPair<KUrl, int> > m_queue;
        int m_resolveMedia;
        int m_attempts;

    signals:
        void urlChanged(KUrl url);
};

}

#endif
