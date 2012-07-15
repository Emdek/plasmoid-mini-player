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

#ifndef MINIPLAYERAPPLET_HEADER
#define MINIPLAYERAPPLET_HEADER

#include <KDialog>

#include <Plasma/Applet>
#include <Plasma/Dialog>

#include "Constants.h"

#include "ui_jumpToPosition.h"
#include "ui_volume.h"

namespace MiniPlayer
{

class PlaylistManager;
class DBusInterface;
class VideoWidget;

class Applet : public Plasma::Applet
{
    Q_OBJECT

    public:
        explicit Applet(QObject *parent, const QVariantList &args);

        void init();
        QList<QAction*> contextualActions();
        Player* player();
        PlaylistManager* playlistManager();
        bool eventFilter(QObject *object, QEvent *event);

    public Q_SLOTS:
        void configChanged();
        void configSave();
        void toolTipAboutToShow();
        void toolTipHidden();
        void showMenu(const QPoint &position);

    protected:
        void createConfigurationInterface(KConfigDialog *parent);
        void constraintsEvent(Plasma::Constraints);
        void resizeEvent(QGraphicsSceneResizeEvent *event);
        void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
        void dragLeaveEvent(QGraphicsSceneDragDropEvent *event);
        void dropEvent(QGraphicsSceneDragDropEvent *event);
        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
        void keyPressEvent(QKeyEvent *event);
        void timerEvent(QTimerEvent *event);

    protected Q_SLOTS:
        void stateChanged(PlayerState state);
        void metaDataChanged();
        void openFiles();
        void openUrl();
        void jumpToPosition();
        void toggleJumpToPosition();
        void toggleVolumeDialog();
        void toggleFullScreen();
        void togglePlaylistDialog();
        void showToolTip();
        void hideToolTip();
        void updateToolTip();
        void updateControls();
        void updateVideo();

    private:
        Player *m_player;
        PlaylistManager *m_playlistManager;
        DBusInterface *m_dBusInterface;
        VideoWidget *m_videoWidget;
        QGraphicsWidget *m_controlsWidget;
        Plasma::Dialog *m_volumeDialog;
        QMap<QString, QGraphicsProxyWidget*> m_controls;
        QList<QAction*> m_actions;
        KDialog *m_jumpToPositionDialog;
        int m_togglePlaylist;
        int m_hideToolTip;
        int m_updateToolTip;
        bool m_initialized;
        Ui::jumpToPosition m_jumpToPositionUi;
        Ui::volume m_volumeUi;
};

}

#endif
