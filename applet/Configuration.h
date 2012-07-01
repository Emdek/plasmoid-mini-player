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

#ifndef MINIPLAYERCONFIGURATION
#define MINIPLAYERCONFIGURATION

#include <KConfigDialog>

#include "ui_general.h"
#include "ui_controls.h"

namespace MiniPlayer
{

class Applet;

class Configuration : public QObject
{
    Q_OBJECT

    public:
        explicit Configuration(Applet *applet, KConfigDialog *parent);

    protected:
        void connectWidgets(QWidget *widget);

    protected Q_SLOTS:
        void save();
        void modify();

    private:
        Applet *m_applet;
        Ui::general m_generalUi;
        Ui::controls m_controlsUi;

    Q_SIGNALS:
        void accepted();
};

}

#endif
