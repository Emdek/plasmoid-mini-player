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

#include "VideoWidget.h"

#include <QtGui/QGraphicsSceneResizeEvent>

#include <KIcon>
#include <KIconLoader>

namespace MiniPlayer
{

VideoWidget::VideoWidget(QGraphicsWidget *parent) : QGraphicsWidget(parent),
   m_videoItem(new QGraphicsVideoItem(this)),
   m_pixmapItem(new QGraphicsPixmapItem(KIcon("applications-multimedia").pixmap(KIconLoader::SizeEnormous), this))
{
    m_videoItem->setPos(14, 14);
    m_videoItem->setSize(size());

    m_pixmapItem->setTransformationMode(Qt::SmoothTransformation);

    showVideo(true);
}

void VideoWidget::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    m_videoItem->setSize(event->newSize());

    qreal scale = qMin(event->newSize().width(), event->newSize().height()) / m_pixmapItem->pixmap().width();

    m_pixmapItem->setScale(scale);
    m_pixmapItem->setPos(((event->newSize().width() - (m_pixmapItem->boundingRect().width() * scale)) / 2), ((event->newSize().height() - (m_pixmapItem->boundingRect().height() * scale)) / 2));
}

void VideoWidget::showVideo(bool show)
{
    m_videoItem->setVisible(show);

    m_pixmapItem->setVisible(!show);

    if (show)
    {
        setGraphicsItem(m_videoItem);
    }
    else
    {
        setGraphicsItem(m_pixmapItem);
    }
}

QGraphicsVideoItem* VideoWidget::videoItem()
{
    return m_videoItem;
}

}
