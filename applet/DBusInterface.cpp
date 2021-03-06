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

#include "DBusInterface.h"
#include "DBusRootAdaptor.h"
#include "DBusTrackListAdaptor.h"
#include "DBusPlayerAdaptor.h"
#include "DBusPlaylistsAdaptor.h"
#include "Applet.h"
#include "Player.h"
#include "PlaylistManager.h"

#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>

#include <unistd.h>

namespace MiniPlayer
{

DBusInterface::DBusInterface(Applet* applet) : QObject(applet)
{
    new DBusRootAdaptor(this, applet->player());
    new DBusTrackListAdaptor(this, applet->player());
    new DBusPlayerAdaptor(this, applet->player());
    new DBusPlaylistsAdaptor(this, applet->playlistManager());

    m_instance = QString("PlasmaMiniPlayer.instance%1_%2").arg(getpid()).arg(applet->id());

    QDBusConnection connection = QDBusConnection::sessionBus();
    connection.registerObject("/org/mpris/MediaPlayer2", this);
    connection.registerService("org.mpris." + m_instance);
    connection.registerService("org.mpris.MediaPlayer2." + m_instance);
}

DBusInterface::~DBusInterface()
{
    QDBusConnection connection = QDBusConnection::sessionBus();
    connection.unregisterService("org.mpris." + m_instance);
    connection.unregisterService("org.mpris.MediaPlayer2." + m_instance);
}

void DBusInterface::updateProperties(const QString &interface, const QVariantMap &properties)
{
    QVariantList arguments;
    arguments << interface;
    arguments << properties;
    arguments << QStringList();

    QDBusMessage message = QDBusMessage::createSignal("/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "PropertiesChanged");
    message.setArguments(arguments);

    QDBusConnection::sessionBus().send(message);
}

}
