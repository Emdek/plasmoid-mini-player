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

#ifndef MINIPLAYERDBUSROOTADAPTOR
#define MINIPLAYERDBUSROOTADAPTOR

#include <QtDBus/QDBusAbstractAdaptor>

namespace MiniPlayer
{

class Player;

class DBusRootAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT

    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")

    Q_PROPERTY(bool CanQuit READ CanQuit)
    Q_PROPERTY(bool Fullscreen READ Fullscreen WRITE setFullscreen)
    Q_PROPERTY(bool CanSetFullscreen READ CanSetFullscreen)
    Q_PROPERTY(bool CanRaise READ CanRaise)
    Q_PROPERTY(bool HasTrackList READ HasTrackList)
    Q_PROPERTY(QString Identity READ Identity)
    Q_PROPERTY(QString DesktopEntry READ DesktopEntry)
    Q_PROPERTY(QStringList SupportedUriSchemes READ SupportedUriSchemes)
    Q_PROPERTY(QStringList SupportedMimeTypes READ SupportedMimeTypes)

    public:
        explicit DBusRootAdaptor(QObject *parent, Player *player);

        QStringList SupportedUriSchemes() const;
        QStringList SupportedMimeTypes() const;
        QString Identity() const;
        QString DesktopEntry() const;
        bool CanQuit() const;
        bool Fullscreen() const;
        bool CanSetFullscreen() const;
        bool CanRaise() const;
        bool HasTrackList() const;

    public slots:
        void Raise() const;
        void Quit() const;
        void setFullscreen(bool enable) const;

    protected slots:
        void emitFullscreenChanged();
        void emitCanSetFullscreenChanged();

    private:
        Player *m_player;
};

}

#endif
