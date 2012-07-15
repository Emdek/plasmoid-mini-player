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

VideoWidget::VideoWidget(QGraphicsWidget *parent) : QGraphicsWidget(parent),
    m_videoWidget(NULL),
    m_iconItem(new QGraphicsPixmapItem(KIcon("applications-multimedia").pixmap(KIconLoader::SizeEnormous), this))
{
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::black);
    palette.setColor(QPalette::Base, Qt::black);

    setPalette(palette);
    setAcceptDrops(true);
    setAutoFillBackground(true);
    setAcceptHoverEvents(true);
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    m_iconItem->show();
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
    if (pipeline && !m_videoWidget)
    {
        QGraphicsView *parentView = NULL;
        QGraphicsView *possibleParentView = NULL;
        const QList<QGraphicsView*> views = (scene()?scene()->views():QList<QGraphicsView*>());

        for (int i = 0; i < views.count(); ++i)
        {
            if (views.at(i)->sceneRect().intersects(sceneBoundingRect()) || views.at(i)->sceneRect().contains(scenePos()))
            {
                if (views.at(i)->isActiveWindow())
                {
                    parentView = views.at(i);

                    break;
                }
                else
                {
                    possibleParentView = views.at(i);
                }
            }
        }

        if (!parentView)
        {
            parentView = possibleParentView;
        }

        if (parentView)
        {
            m_videoWidget = new QGst::Ui::GraphicsVideoWidget(this);
            m_videoWidget->setSurface(new QGst::Ui::GraphicsVideoSurface(parentView));

            QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(this);
            layout->addItem(m_videoWidget);
        }
        else
        {
            return;
        }
    }

    if (pipeline && m_videoWidget)
    {
        m_videoWidget->show();
        m_iconItem->hide();

        pipeline->setProperty("video-sink", m_videoWidget->surface()->videoSink());
    }
    else
    {
        m_iconItem->show();

        if (m_videoWidget)
        {
            m_videoWidget->hide();
        }
    }
}

}
