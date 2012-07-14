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

#include <QtGui/QGraphicsLinearLayout>
#include <QtGui/QGraphicsSceneResizeEvent>

#include <KIcon>
#include <KIconLoader>

namespace MiniPlayer
{

VideoWidget::VideoWidget(QGraphicsView *view, QGraphicsWidget *parent) : QGraphicsWidget(parent),
    m_videoWidget(new QGst::Ui::GraphicsVideoWidget(this)),
    m_iconItem(new QGraphicsPixmapItem(KIcon("applications-multimedia").pixmap(KIconLoader::SizeEnormous), this))
{
    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(this);
    layout->addItem(m_videoWidget);

    if (view)
    {
        m_videoWidget->setSurface(new QGst::Ui::GraphicsVideoSurface(view));
    }

    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::black);
    palette.setColor(QPalette::Base, Qt::black);

    setPalette(palette);
    setAcceptDrops(true);
    setAutoFillBackground(true);
    setAcceptHoverEvents(true);
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    m_iconItem->setTransformationMode(Qt::SmoothTransformation);
    m_iconItem->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
    m_iconItem->setZValue(100);
}

void VideoWidget::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    const qreal scale = qMin(event->newSize().width(), event->newSize().height()) / m_iconItem->pixmap().width();

    m_iconItem->setScale(scale);
    m_iconItem->setPos(((event->newSize().width() - (m_iconItem->boundingRect().width() * scale)) / 2), ((event->newSize().height() - (m_iconItem->boundingRect().height() * scale)) / 2));
}

void VideoWidget::setPipeline(QGst::PipelinePtr pipeline)
{
    if (pipeline && m_videoWidget->surface())
    {
        m_videoWidget->show();
        m_iconItem->hide();

        pipeline->setProperty("video-sink", m_videoWidget->surface()->videoSink());
    }
    else
    {
        m_iconItem->show();
        m_videoWidget->hide();
    }
}

}
