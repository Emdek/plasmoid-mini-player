/******************************************************************************
* Copyright (C) 2008 Ian Monroe <ian@monroe.nu>
* Copyright (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>
* Copyright (C) 2009 - 2012 by Michal Dutkiewicz <emdeck@gmail.com>
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

#include "RootDBusHandler.h"
#include "RootAdaptor.h"

QDBusArgument &operator << (QDBusArgument &argument, const Version &version)
{
    argument.beginStructure();
    argument << version.major << version.minor;
    argument.endStructure();

    return argument;
}

const QDBusArgument &operator >> (const QDBusArgument &argument, Version &version)
{
    argument.beginStructure();
    argument >> version.major >> version.minor;
    argument.endStructure();

    return argument;
}

RootDBusHandler::RootDBusHandler(QObject *parent) : QObject(parent)
{
    qDBusRegisterMetaType<Version>();

    setObjectName("RootDBusHandler");

    new RootAdaptor(this);

    QDBusConnection::sessionBus().registerObject("/", this);
}

QString RootDBusHandler::Identity()
{
    return QString("%1 %2").arg("PlasmaMiniPlayer", 1.0);
}

void RootDBusHandler::Quit()
{
}

Version RootDBusHandler::MprisVersion()
{
    struct Version version;
    version.major = 1;
    version.minor = 0;

    return version;
}
