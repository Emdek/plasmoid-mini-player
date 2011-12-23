/***********************************************************************************
* Mini Player: Advanced media player for Plasma.
* Copyright (C) 2008 - 2011 Michal Dutkiewicz aka Emdek <emdeck@gmail.com>
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
#include "MetaDataManager.h"

#include <QtGui/QStyle>
#include <QtGui/QMouseEvent>
#include <QtGui/QStyleOptionSlider>

namespace MiniPlayer
{

SeekSlider::SeekSlider(QWidget *parent) : QSlider(parent),
    m_mediaPlayer(NULL),
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
    if (!m_mediaPlayer)
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
        position = (((float) (event->x()  - (handle.width() / 2)) / groove.width()) * m_mediaPlayer->duration());
    }
    else
    {
        position = (((float) (event->y() - (handle.height() / 2)) / groove.height()) * m_mediaPlayer->duration());
    }

    setToolTip(position?MetaDataManager::timeToString(position):"0:00:00");

    QSlider::mouseMoveEvent(event);
}

void SeekSlider::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event)

    if (!isSliderDown() && m_mediaPlayer)
    {
        disconnect(this, SIGNAL(valueChanged(int)), this, SLOT(positionChanged(int)));

        setValue((m_mediaPlayer->position() * 10000) / m_mediaPlayer->duration());

        connect(this, SIGNAL(valueChanged(int)), this, SLOT(positionChanged(int)));
    }
}

void SeekSlider::setMediaPlayer(QMediaPlayer *mediaPlayer)
{
    if (m_mediaPlayer)
    {
        disconnect(m_mediaPlayer, SIGNAL(seekableChanged(bool)), this, SLOT(mediaChanged()));
        disconnect(m_mediaPlayer, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(mediaChanged()));
    }

    m_mediaPlayer = mediaPlayer;

    if (!m_mediaPlayer)
    {
        setEnabled(false);
        setToolTip(QString());
        triggerAction(QAbstractSlider::SliderToMinimum);

        return;
    }

    mediaChanged();

    connect(m_mediaPlayer, SIGNAL(seekableChanged(bool)), this, SLOT(mediaChanged()));
    connect(m_mediaPlayer, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(mediaChanged()));
}

void SeekSlider::positionChanged(int position)
{
    if (m_mediaPlayer)
    {
        killTimer(m_updatePosition);

        m_mediaPlayer->setPosition((m_mediaPlayer->duration() * position) / 10000);

        m_updatePosition = startTimer(250);
    }
}

void SeekSlider::mediaChanged()
{
    if (!m_mediaPlayer)
    {
        return;
    }

    if (m_mediaPlayer->isSeekable() && (m_mediaPlayer->state() == QMediaPlayer::PlayingState || m_mediaPlayer->state() == QMediaPlayer::PausedState))
    {
        setEnabled(true);
        setSingleStep(qMin((qint64) 1, m_mediaPlayer->duration() / 300000));
        setPageStep(qMin((qint64) 1, m_mediaPlayer->duration() / 30000));

        m_updatePosition = startTimer(250);
    }
    else
    {
        setEnabled(false);
        setToolTip(QString());
        triggerAction(QAbstractSlider::SliderToMinimum);

        killTimer(m_updatePosition);
    }
}

}
