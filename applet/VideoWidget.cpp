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

#include "VideoWidget.h"

#include <QtGui/QGraphicsSceneResizeEvent>

#include <KIcon>
#include <KIconLoader>

namespace MiniPlayer
{

VideoWidget::VideoWidget(QGraphicsWidget *parent) : QGraphicsProxyWidget(parent),
   m_pixmapItem(new QGraphicsPixmapItem(KIcon("applications-multimedia").pixmap(KIconLoader::SizeEnormous), this)),
   m_backgroundWidget(new QGraphicsWidget(this)),
   m_updateTimer(0)
{
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::black);
    palette.setColor(QPalette::Base, Qt::black);

    setPalette(palette);
    setAcceptDrops(true);
    setAutoFillBackground(true);
    setAcceptHoverEvents(true);
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    m_pixmapItem->setTransformationMode(Qt::SmoothTransformation);
    m_pixmapItem->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
    m_pixmapItem->setZValue(100);

    m_backgroundWidget->setPalette(palette);
    m_backgroundWidget->setAutoFillBackground(true);
    m_backgroundWidget->setZValue(50);
}

void VideoWidget::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    qreal scale = qMin(event->newSize().width(), event->newSize().height()) / m_pixmapItem->pixmap().width();

    if (widget())
    {
        widget()->resize(event->newSize().toSize());
    }

    m_pixmapItem->setScale(scale);
    m_pixmapItem->setPos(((event->newSize().width() - (m_pixmapItem->boundingRect().width() * scale)) / 2), ((event->newSize().height() - (m_pixmapItem->boundingRect().height() * scale)) / 2));

    m_backgroundWidget->resize(event->newSize());
    m_backgroundWidget->setPos(0, 0);
}

void VideoWidget::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event)

    update();
}

void VideoWidget::setVideoWidget(Phonon::VideoWidget *videoWidget, bool mode)
{
    killTimer(m_updateTimer);

    if (videoWidget)
    {
        const QSize size = this->size().toSize();

        setWidget(videoWidget);

        videoWidget->show();
        videoWidget->resize(size);

        m_updateTimer = startTimer(50);
    }
    else
    {
        setWidget(NULL);
    }

    show();

    m_pixmapItem->setVisible(!mode);

    m_backgroundWidget->setVisible(!mode);
}

}
