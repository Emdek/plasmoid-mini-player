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

#include "SeekSlider.h"
#include "Player.h"
#include "MetaDataManager.h"

#include <QtGui/QStyle>
#include <QtGui/QMouseEvent>
#include <QtGui/QStyleOptionSlider>

namespace MiniPlayer
{

SeekSlider::SeekSlider(QWidget *parent) : QSlider(parent),
    m_player(NULL),
    m_updatePosition(0),
    m_isDragged(false)
{
    setEnabled(false);
    setRange(0, 10000);
    setTracking(true);
    setMouseTracking(true);
    setOrientation(Qt::Horizontal);
    triggerAction(QAbstractSlider::SliderToMinimum);

    connect(this, SIGNAL(valueChanged(int)), this, SLOT(positionChanged(int)));
}

void SeekSlider::mousePressEvent(QMouseEvent *event)
{
    QSlider::mousePressEvent(event);

    if (!isSliderDown() && event->button() == Qt::LeftButton)
    {
        QStyleOptionSlider option;

        initStyleOption(&option);

        QRect groove = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderGroove, this);
        QRect handle = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, this);
        int position;

        killTimer(m_updatePosition);

        if (orientation() == Qt::Horizontal)
        {
            position = QStyle::sliderValueFromPosition(0, 10000, (event->x() - (handle.width() / 2) - groove.x()), (groove.right() - handle.width()));
        }
        else
        {
            position = QStyle::sliderValueFromPosition(0, 10000, (event->y() - (handle.height() / 2) - groove.y()), (groove.bottom() - handle.height()), true);
        }

        setValue(position);
    }
}

void SeekSlider::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_player || m_player->duration() < 1)
    {
        return;
    }

    QStyleOptionSlider option;

    initStyleOption(&option);

    QRect groove = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderGroove, this);
    QRect handle = style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, this);
    qint64 position;

    if (orientation() == Qt::Horizontal)
    {
        position = (((float) (event->x()  - (handle.width() / 2)) / groove.width()) * m_player->duration());
    }
    else
    {
        position = (((float) (event->y() - (handle.height() / 2)) / groove.height()) * m_player->duration());
    }

    setToolTip(position?MetaDataManager::timeToString(position):"0:00:00");

    QSlider::mouseMoveEvent(event);
}

void SeekSlider::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event)

    if (!m_player)
    {
        return;
    }

    const int position = ((m_player->duration() > 0)?((m_player->position() * 10000) / m_player->duration()):0);

    if (!isSliderDown() && position != value())
    {
        disconnect(this, SIGNAL(valueChanged(int)), this, SLOT(positionChanged(int)));

        setValue(position);

        connect(this, SIGNAL(valueChanged(int)), this, SLOT(positionChanged(int)));
    }
}

void SeekSlider::setPlayer(Player *player)
{
    if (m_player)
    {
        disconnect(m_player, SIGNAL(trackChanged()), this, SLOT(mediaChanged()));
        disconnect(m_player, SIGNAL(seekableChanged(bool)), this, SLOT(mediaChanged()));
        disconnect(m_player, SIGNAL(stateChanged(PlayerState)), this, SLOT(mediaChanged()));
    }

    m_player = player;

    if (!m_player)
    {
        setEnabled(false);
        setToolTip(QString());
        triggerAction(QAbstractSlider::SliderToMinimum);

        return;
    }

    mediaChanged();

    connect(m_player, SIGNAL(trackChanged()), this, SLOT(mediaChanged()));
    connect(m_player, SIGNAL(seekableChanged(bool)), this, SLOT(mediaChanged()));
    connect(m_player, SIGNAL(stateChanged(PlayerState)), this, SLOT(mediaChanged()));
}

void SeekSlider::positionChanged(int position)
{
    if (m_player)
    {
        killTimer(m_updatePosition);

        m_player->setPosition((m_player->duration() * position) / 10000);

        m_updatePosition = startTimer(250);
    }
}

void SeekSlider::mediaChanged()
{
    if (!m_player)
    {
        return;
    }

    killTimer(m_updatePosition);

    if (m_player->isSeekable())
    {
        setEnabled(true);
        setSingleStep(qMin((qint64) 1, (m_player->duration() / 300000)));
        setPageStep(qMin((qint64) 1, (m_player->duration() / 30000)));
    }
    else
    {
        setEnabled(false);
        setToolTip(QString());
    }

    if (m_player->position() < 1)
    {
        triggerAction(QAbstractSlider::SliderToMinimum);
    }
    else
    {
        m_updatePosition = startTimer(250);
    }
}

}
