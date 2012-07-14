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

#ifndef MINIPLAYERMETADATAMANAGER_HEADER
#define MINIPLAYERMETADATAMANAGER_HEADER

#include <QtCore/QQueue>

#include <KUrl>
#include <KIcon>
#include <KLocale>

#include <QGst/Message>
#include <QGst/Pipeline>

#include "Constants.h"

namespace MiniPlayer
{

struct Track
{
    QMap<MetaDataKey, QString> keys;
    qint64 duration;
};

class MetaDataManager : public QObject
{
    Q_OBJECT

    public:
        static void createInstance(QObject *parent = NULL);
        static void resolveTracks(const KUrl::List &urls);
        static void setDuration(const KUrl &url, qint64 duration);
        static void setMetaData(const KUrl &url, MetaDataKey key, const QString &value);
        static void setMetaData(const KUrl &url, const Track &track);
        static void removeMetaData(const KUrl &url);
        static MetaDataManager* instance();
        static KUrl::List tracks();
        static QVariantMap metaData(const KUrl &url);
        static QString metaData(const KUrl &url, MetaDataKey key, bool substitute = true);
        static QString timeToString(qint64 time);
        static QString urlToTitle(const KUrl &url);
        static KIcon icon(const KUrl &url);
        static qint64 duration(const KUrl &url);
        static bool isAvailable(const KUrl &url);

    protected:
        explicit MetaDataManager(QObject *parent);

        void timerEvent(QTimerEvent *event);
        void handleBusMessage(const QGst::MessagePtr &message);
        void scheduleNext();
        void addTracks(const KUrl::List &urls);
        void setMetaData(const KUrl &url, const Track &track, bool notify);

    private:
        QGst::PipelinePtr m_pipeline;
        KUrl m_url;
        int m_attempts;
        int m_scheduleNextTimer;

        static QQueue<QPair<KUrl, int> > m_queue;
        static QMap<KUrl, Track> m_tracks;
        static MetaDataManager *m_instance;

    Q_SIGNALS:
        void urlChanged(KUrl url);
};

}

#endif
