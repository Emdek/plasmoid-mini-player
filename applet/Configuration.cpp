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

#include "Configuration.h"
#include "Applet.h"

namespace MiniPlayer
{

Configuration::Configuration(Applet *applet, KConfigDialog *parent) : QObject(parent),
    m_applet(applet)
{
    KConfigGroup configuration = m_applet->config();
    QWidget *generalWidget = new QWidget;
    QWidget *controlsWidget = new QWidget;
    QStringList controls;
    controls << "open" << "playPause" << "stop" << "position" << "volume" << "playlist";
    controls = configuration.readEntry("controls", controls);

    m_generalUi.setupUi(generalWidget);
    m_controlsUi.setupUi(controlsWidget);

    connectWidgets(generalWidget);
    connectWidgets(controlsWidget);

    m_generalUi.startPlaybackCheckBox->setChecked(configuration.readEntry("playOnStartup", false));
    m_generalUi.dbusCheckBox->setChecked(configuration.readEntry("enableDBus", false));
    m_generalUi.inhibitNotificationsCheckBox->setChecked(configuration.readEntry("inhibitNotifications", false));
    m_generalUi.showTooltipOnTrackChange->setValue(configuration.readEntry("showToolTipOnTrackChange", 3));

    m_controlsUi.openCheckBox->setChecked(controls.contains("open"));
    m_controlsUi.playPauseCheckBox->setChecked(controls.contains("playPause"));
    m_controlsUi.stopCheckBox->setChecked(controls.contains("stop"));
    m_controlsUi.playPreviousCheckBox->setChecked(controls.contains("playPrevious"));
    m_controlsUi.playNextCheckBox->setChecked(controls.contains("playNext"));
    m_controlsUi.positionCheckBox->setChecked(controls.contains("position"));
    m_controlsUi.volumeCheckBox->setChecked(controls.contains("volume"));
    m_controlsUi.muteCheckBox->setChecked(controls.contains("mute"));
    m_controlsUi.playlistCheckBox->setChecked(controls.contains("playlist"));
    m_controlsUi.fullScreenCheckBox->setChecked(controls.contains("fullScreen"));

    parent->addPage(generalWidget, i18n("General"), "go-home");
    parent->addPage(controlsWidget, i18n("Visible Controls"), "media-playback-start");

    connect(parent, SIGNAL(applyClicked()), this, SLOT(save()));
    connect(parent, SIGNAL(okClicked()), this, SLOT(save()));
    connect(m_generalUi.showTooltipOnTrackChange, SIGNAL(valueChanged(int)), this, SLOT(modify()));
}

void Configuration::save()
{
    KConfigGroup configuration = m_applet->config();
    QStringList controls;

    if (m_controlsUi.openCheckBox->isChecked())
    {
        controls.append("open");
    }

    if (m_controlsUi.playPauseCheckBox->isChecked())
    {
        controls.append("playPause");
    }

    if (m_controlsUi.stopCheckBox->isChecked())
    {
        controls.append("stop");
    }

    if (m_controlsUi.playPreviousCheckBox->isChecked())
    {
        controls.append("playPrevious");
    }

    if (m_controlsUi.playNextCheckBox->isChecked())
    {
        controls.append("playNext");
    }

    if (m_controlsUi.positionCheckBox->isChecked())
    {
        controls.append("position");
    }

    if (m_controlsUi.volumeCheckBox->isChecked())
    {
        controls.append("volume");
    }

    if (m_controlsUi.muteCheckBox->isChecked())
    {
        controls.append("mute");
    }

    if (m_controlsUi.playlistCheckBox->isChecked())
    {
        controls.append("playlist");
    }

    if (m_controlsUi.fullScreenCheckBox->isChecked())
    {
        controls.append("fullScreen");
    }

    configuration.writeEntry("controls", controls);
    configuration.writeEntry("playOnStartup", m_generalUi.startPlaybackCheckBox->isChecked());
    configuration.writeEntry("enableDBus", m_generalUi.dbusCheckBox->isChecked());
    configuration.writeEntry("inhibitNotifications", m_generalUi.inhibitNotificationsCheckBox->isChecked());
    configuration.writeEntry("showToolTipOnTrackChange", m_generalUi.showTooltipOnTrackChange->value());

    static_cast<KConfigDialog*>(parent())->enableButtonApply(false);

    emit accepted();
}

void Configuration::modify()
{
    static_cast<KConfigDialog*>(parent())->enableButtonApply(true);
}

void Configuration::connectWidgets(QWidget *widget)
{
    QList<QAbstractButton*> buttons = widget->findChildren<QAbstractButton*>();

    for (int i = 0; i < buttons.count(); ++i)
    {
        connect(buttons.at(i), SIGNAL(toggled(bool)), this, SLOT(modify()));
    }
}

}
