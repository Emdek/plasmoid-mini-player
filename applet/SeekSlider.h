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

#ifndef MINIPLAYERSEEKSLIDER_HEADER
#define MINIPLAYERSEEKSLIDER_HEADER

#include <QtGui/QSlider>

namespace MiniPlayer
{

class Player;

class SeekSlider : public QSlider
{
    Q_OBJECT

    public:
        explicit SeekSlider(QWidget *parent = NULL);

        void setPlayer(Player *player);

    public slots:
        void positionChanged(int position);
        void mediaChanged();

    protected:
        void mousePressEvent(QMouseEvent *event);
        void mouseMoveEvent(QMouseEvent *event);
        void timerEvent(QTimerEvent *event);

    private:
        Player *m_player;
        int m_updatePosition;
        bool m_isDragged;
};

}

#endif
