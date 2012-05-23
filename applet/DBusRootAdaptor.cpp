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

#include "DBusRootAdaptor.h"
#include "DBusInterface.h"
#include "Player.h"

#include <KProtocolInfo>

namespace MiniPlayer
{

DBusRootAdaptor::DBusRootAdaptor(QObject *parent, Player *player) : QDBusAbstractAdaptor(parent),
    m_player(player)
{
    connect(m_player, SIGNAL(fullScreenChanged(bool)), this, SLOT(emitFullscreenChanged()));
    connect(m_player, SIGNAL(videoAvailableChanged(bool)), this, SLOT(emitCanSetFullscreenChanged()));
}

void DBusRootAdaptor::Raise() const
{
}

void DBusRootAdaptor::Quit() const
{
}

void DBusRootAdaptor::setFullscreen(bool enable) const
{
    m_player->setFullScreen(enable);
}

void DBusRootAdaptor::emitFullscreenChanged()
{
    QVariantMap properties;
    properties["Fullscreen"] = Fullscreen();

    DBusInterface::updateProperties("org.mpris.MediaPlayer2", properties);
}

void DBusRootAdaptor::emitCanSetFullscreenChanged()
{
    QVariantMap properties;
    properties["CanSetFullscreen"] = CanSetFullscreen();

    DBusInterface::updateProperties("org.mpris.MediaPlayer2", properties);
}

QStringList DBusRootAdaptor::SupportedUriSchemes() const
{
    QStringList protocols;

    foreach (const QString &protocol, KProtocolInfo::protocols())
    {
        if (!KProtocolInfo::isHelperProtocol(protocol))
        {
            protocols.append(protocol);
        }
    }

    return protocols;
}

QStringList DBusRootAdaptor::SupportedMimeTypes() const
{
    return m_player->supportedMimeTypes();
}

QString DBusRootAdaptor::Identity() const
{
    return "Plasma Mini Player";
}

QString DBusRootAdaptor::DesktopEntry() const
{
    return QString();
}

bool DBusRootAdaptor::CanQuit() const
{
    return false;
}

bool DBusRootAdaptor::Fullscreen() const
{
    return m_player->isFullScreen();
}

bool DBusRootAdaptor::CanSetFullscreen() const
{
    return m_player->isVideoAvailable();
}

bool DBusRootAdaptor::CanRaise() const
{
    return false;
}

bool DBusRootAdaptor::HasTrackList() const
{
    return true;
}

}
