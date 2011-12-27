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

#ifndef MINIPLAYERVIDEOWIDGET_HEADER
#define MINIPLAYERVIDEOWIDGET_HEADER

#include <QtGui/QGraphicsWidget>
#include <QtGui/QGraphicsPixmapItem>
#include <QtMultimediaKit/QGraphicsVideoItem>

namespace MiniPlayer
{

class Player;

class VideoWidget : public QGraphicsWidget
{
    Q_OBJECT

    public:
        VideoWidget(QGraphicsWidget *parent);

    protected slots:
        void showVideo(bool show);

    protected:
        void resizeEvent(QGraphicsSceneResizeEvent *event);
        QGraphicsVideoItem* videoItem();

    private:
        QGraphicsVideoItem *m_videoItem;
        QGraphicsPixmapItem *m_pixmapItem;

    friend class Player;
};

}

#endif
