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
#include <QtCore/QVariantMap>
#include <QtGui/QKeyEvent>
#include <QtMultimediaKit/QMediaPlayer>
#include <QtMultimediaKit/QMediaPlaylist>

#include <KDialog>
#include <KNotificationRestrictions>

#include <Plasma/Applet>
#include <Plasma/Dialog>

#include "ui_general.h"
#include "ui_controls.h"
#include "ui_jumpToPosition.h"
#include "ui_playlist.h"
#include "ui_fullScreen.h"
#include "ui_volume.h"

class PlayerDBusHandler;
class TrackListDBusHandler;
class RootDBusHandler;

namespace MiniPlayer
{

class VideoWidget;
class MetaDataManager;
class PlaylistModel;

class Applet : public Plasma::Applet
{
    Q_OBJECT

    public:
        Applet(QObject *parent, const QVariantList &args);
        ~Applet();

        void init();
        void addToPlaylist(const KUrl::List &items, bool play, int index = -1);
        void createPlaylist(const QString &playlist, const KUrl::List &tracks = KUrl::List());
        QList<QAction*> contextualActions();
        QMediaPlayer* mediaPlayer();
        MetaDataManager* metaDataManager();
        QMediaPlaylist* playlist();
        QMediaPlaylist::PlaybackMode playbackMode() const;
        bool eventFilter(QObject *object, QEvent *event);

    public slots:
        void configAccepted();
        void configReset();
        void stateChanged(QMediaPlayer::State state);
        void errorOccured(QMediaPlayer::Error error);
        void videoAvailableChanged(bool videoAvailable);
        void volumeChanged();
        void metaDataChanged();
        void trackPressed();
        void trackChanged();
        void moveUpTrack();
        void moveDownTrack();
        void playTrack(QModelIndex index = QModelIndex());
        void editTrackTitle();
        void copyTrackUrl();
        void removeTrack();
        void openFiles(bool playFirst = true);
        void openUrl();
        void play(int index);
        void play(KUrl url);
        void playPause();
        void playPrevious();
        void playNext();
        void play();
        void pause();
        void stop();
        void seekBackward();
        void seekForward();
        void increaseVolume();
        void decreaseVolume();
        void jumpToPosition();
        void toggleJumpToPosition();
        void toggleVolumeDialog();
        void togglePlaylistDialog();
        void toggleFullScreen();
        void toggleMute();
        void filterPlaylist(const QString &text);
        void savePlaylistNames();
        void movePlaylist(int from, int to);
        void renamePlaylist(int position);
        void removePlaylist(int position);
        void visiblePlaylistChanged(int position);
        void savePlaylist();
        void addToPlaylist();
        void exportPlaylist();
        void newPlaylist();
        void clearPlaylist();
        void shufflePlaylist();
        void setCurrentPlaylist(const QString &playlist);
        void setAspectRatio(int ratio);
        void setPlaybackMode(int mode);
        void changeAspectRatio(QAction *action);
        void changePlaybackMode(QAction *action);
        void showToolTip();
        void updateToolTip();
        void toolTipAboutToShow();
        void toolTipHidden();
        void savePlaylistSettings(int position = 0, int index = 0);
        void importPlaylist(const QString &playlist, const KUrl::List &tracks, const QHash<KUrl, QPair<QString, qint64> > &metaData, bool play, int index);
        void updateTheme();

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
        void updateControls(QMediaPlayer::State state);
        void updateVideoWidgets();

    private:
        QHash<QString, PlaylistModel*> m_playlists;
        QMediaPlayer *m_mediaPlayer;
        MetaDataManager *m_metaDataManager;
        VideoWidget *m_videoWidget;
        Plasma::Dialog *m_volumeDialog;
        Plasma::Dialog *m_playlistDialog;
        PlayerDBusHandler *m_playerDBUSHandler;
        TrackListDBusHandler *m_trackListDBusHandler;
        RootDBusHandler *m_rootDBUSHandler;
        QHash<QString, QGraphicsWidget*> m_controls;
        QList<QAction*> m_actions;
        QGraphicsWidget *m_controlsWidget;
        QWidget *m_fullScreenWidget;
        QString m_currentPlaylist;
        QString m_visiblePlaylist;
        QString m_title;
        KDialog *m_jumpToPositionDialog;
        QAction *m_audioAction;
        QAction *m_volumeAction;
        QAction *m_playPauseAction;
        QAction *m_stopAction;
        QAction *m_navigationAction;
        QAction *m_fullScreenAction;
        QAction *m_playlistAction;
        QAction *m_muteAction;
        QAction *m_videoAction;
        QMenu *m_openMenu;
        QMenu *m_aspectRatioMenu;
        QMenu *m_playbackModeMenu;
        KNotificationRestrictions *m_notificationRestrictions;
        int m_hideFullScreenControls;
        int m_showPlaylist;
        int m_hideToolTip;
        int m_stopSleepCookie;
        bool m_editorActive;
        bool m_updateToolTip;
        bool m_videoMode;
        Ui::general m_generalUi;
        Ui::controls m_controlsUi;
        Ui::jumpToPosition m_jumpToPositionUi;
        Ui::playlist m_playlistUi;
        Ui::fullScreen m_fullScreenUi;
        Ui::volume m_volumeUi;

    signals:
        void resetModel();
};

}

#endif
