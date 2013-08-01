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

#include "VolumeSlider.h"
#include "Player.h"

#include <QtGui/QStyle>
#include <QtGui/QMouseEvent>
#include <QtGui/QStyleOptionSlider>

#include <KLocale>

namespace MiniPlayer
{

VolumeSlider::VolumeSlider(QWidget *parent) : QSlider(parent),
    m_player(NULL)
{
    setEnabled(false);
    setRange(0, 100);
    setTracking(true);
    setOrientation(Qt::Vertical);
    triggerAction(QAbstractSlider::SliderToMinimum);
}

void VolumeSlider::mousePressEvent(QMouseEvent *event)
{
    QSlider::mousePressEvent(event);

    if (!isSliderDown() && event->button() == Qt::LeftButton)
    {
        QStyleOptionSlider option;

        initStyleOption(&option);

        const QRect groove = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderGroove, this);
        const QRect handle = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, this);
        int position;

        if (orientation() == Qt::Horizontal)
        {
            position = QStyle::sliderValueFromPosition(0, 100, (event->x() - (handle.width() / 2) - groove.x()), (groove.right() - handle.width()));
        }
        else
        {
            position = QStyle::sliderValueFromPosition(0, 100, (event->y() - (handle.height() / 2) - groove.y()), (groove.bottom() - handle.height()), true);
        }

        setValue(position);
    }
}

void VolumeSlider::setPlayer(Player *player)
{
    if (m_player)
    {
        disconnect(this, SIGNAL(valueChanged(int)), m_player, SLOT(setVolume(int)));
        disconnect(m_player, SIGNAL(volumeChanged(int)), this, SLOT(volumeChanged(int)));
        disconnect(m_player, SIGNAL(audioAvailableChanged(bool)), this, SLOT(setEnabled(bool)));
    }

    m_player = player;

    if (!m_player)
    {
        triggerAction(QAbstractSlider::SliderToMinimum);

        return;
    }

    setEnabled(m_player->isAudioAvailable());
    setValue(m_player->volume());

    connect(this, SIGNAL(valueChanged(int)), m_player, SLOT(setVolume(int)));
    connect(m_player, SIGNAL(volumeChanged(int)), this, SLOT(volumeChanged(int)));
    connect(m_player, SIGNAL(audioAvailableChanged(bool)), this, SLOT(setEnabled(bool)));
}

void VolumeSlider::volumeChanged(int volume)
{
    if (!m_player)
    {
        return;
    }

    disconnect(this, SIGNAL(valueChanged(int)), m_player, SLOT(setVolume(int)));

    setValue(volume);

    connect(this, SIGNAL(valueChanged(int)), m_player, SLOT(setVolume(int)));

    setToolTip(m_player->isAudioMuted()?i18n("Muted"):i18n("Volume: %1%", m_player->volume()));
}

}
