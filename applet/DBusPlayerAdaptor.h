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

#ifndef MINIPLAYERDBUSPLAYERADAPTOR
#define MINIPLAYERDBUSPLAYERADAPTOR

#include <QtCore/QVariantMap>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusAbstractAdaptor>

namespace MiniPlayer
{

class Player;

class DBusPlayerAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT

    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")

    Q_PROPERTY(QString PlaybackStatus READ PlaybackStatus)
    Q_PROPERTY(QString LoopStatus READ LoopStatus WRITE setLoopStatus)
    Q_PROPERTY(double Rate READ Rate WRITE setRate)
    Q_PROPERTY(bool Shuffle READ Shuffle WRITE setShuffle)
    Q_PROPERTY(QVariantMap Metadata READ Metadata)
    Q_PROPERTY(double Volume READ Volume WRITE setVolume)
    Q_PROPERTY(qint64 Position READ Position)
    Q_PROPERTY(double MinimumRate READ MinimumRate)
    Q_PROPERTY(double MaximumRate READ MaximumRate)
    Q_PROPERTY(bool CanGoNext READ CanGoNext)
    Q_PROPERTY(bool CanGoPrevious READ CanGoPrevious)
    Q_PROPERTY(bool CanPlay READ CanPlay)
    Q_PROPERTY(bool CanPause READ CanPause)
    Q_PROPERTY(bool CanSeek READ CanSeek)
    Q_PROPERTY(bool CanControl READ CanControl)

    public:
        explicit DBusPlayerAdaptor(QObject *parent, Player *player);

        void setLoopStatus(const QString& loopStatus) const;
        void setRate(double rate) const;
        void setShuffle(bool shuffle) const;
        void setVolume(double volume) const;
        QVariantMap Metadata() const;
        QString PlaybackStatus() const;
        QString LoopStatus() const;
        qint64 Position() const;
        double Rate() const;
        double Volume() const;
        double MinimumRate() const;
        double MaximumRate() const;
        bool Shuffle() const;
        bool CanGoNext() const;
        bool CanGoPrevious() const;
        bool CanPlay() const;
        bool CanPause() const;
        bool CanSeek() const;
        bool CanControl() const;

    public slots:
        void Next() const;
        void Previous() const;
        void Pause() const;
        void PlayPause() const;
        void Stop() const;
        void Play() const;
        void Seek(qint64 offset) const;
        void SetPosition(const QDBusObjectPath &trackId, qint64 position) const;
        void OpenUri(QString uri) const;

    protected slots:
        void updateProperties();
        void emitMetaDataChanged();
        void emitSeeked(qint64 position);

    private:
        QVariantMap m_properties;
        Player *m_player;

    signals:
        void Seeked(qint64 position) const;
};

}

#endif
