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

#ifndef MINIPLAYERAPPLET_HEADER
#define MINIPLAYERAPPLET_HEADER

#include <QtCore/QTimerEvent>
#include <QtGui/QKeyEvent>

#include <KDialog>
#include <KNotificationRestrictions>

#include <Plasma/Applet>
#include <Plasma/Dialog>

#include "Player.h"

#include "ui_general.h"
#include "ui_controls.h"
#include "ui_jumpToPosition.h"
#include "ui_fullScreen.h"
#include "ui_volume.h"

class PlayerDBusHandler;
class TrackListDBusHandler;
class RootDBusHandler;

namespace MiniPlayer
{

class PlaylistManager;
class VideoWidget;

class Applet : public Plasma::Applet
{
    Q_OBJECT

    public:
        Applet(QObject *parent, const QVariantList &args);
        ~Applet();

        void init();
        QList<QAction*> contextualActions();
        Player* player();
        bool eventFilter(QObject *object, QEvent *event);

    public slots:
        void configAccepted();
        void configSave();
        void configReset();
        void stateChanged(PlayerState state);
        void videoAvailableChanged(bool videoAvailable);
        void metaDataChanged();
        void openFiles();
        void openUrl();
        void jumpToPosition();
        void toggleJumpToPosition();
        void toggleVolumeDialog();
        void toggleFullScreen();
        void togglePlaylistDialog();
        void showToolTip();
        void updateToolTip();
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

    private:
        Player *m_player;
        PlaylistManager *m_playlistManager;
        VideoWidget *m_videoWidget;
        Plasma::Dialog *m_volumeDialog;
        PlayerDBusHandler *m_playerDBUSHandler;
        TrackListDBusHandler *m_trackListDBusHandler;
        RootDBusHandler *m_rootDBUSHandler;
        QHash<QString, QGraphicsWidget*> m_controls;
        QList<QAction*> m_actions;
        QGraphicsWidget *m_controlsWidget;
        QWidget *m_fullScreenWidget;
        KDialog *m_jumpToPositionDialog;
        KNotificationRestrictions *m_notificationRestrictions;
        int m_hideFullScreenControls;
        int m_showPlaylist;
        int m_hideToolTip;
        int m_stopSleepCookie;
        bool m_updateToolTip;
        Ui::general m_generalUi;
        Ui::controls m_controlsUi;
        Ui::jumpToPosition m_jumpToPositionUi;
        Ui::fullScreen m_fullScreenUi;
        Ui::volume m_volumeUi;

    signals:
        void resetModel();
        void titleChanged(QString title);
};

}

#endif
