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

#ifndef MINIPLAYERPLAYER_HEADER
#define MINIPLAYERPLAYER_HEADER

#include <QtCore/QPointer>
#include <QtGui/QAction>
#include <QtGui/QSlider>
#include <QtGui/QActionGroup>

#include <QGst/Message>
#include <QGst/Pipeline>

#include <KUrl>
#include <KNotificationRestrictions>

#include "Constants.h"

#include "ui_fullScreen.h"

namespace MiniPlayer
{

class PlaylistModel;
class VideoWidget;

class Player : public QObject
{
    Q_OBJECT

    public:
        explicit Player(QObject *parent);
        ~Player();

        void registerAppletVideoWidget(VideoWidget *videoWidget);
        void registerDialogVideoWidget(VideoWidget *videoWidget);
        PlaylistModel* playlist() const;
        QAction* action(PlayerAction action) const;
        QStringList supportedMimeTypes() const;
        QString metaData(MetaDataKey key, bool substitute = true) const;
        QVariantMap metaData() const;
        KUrl url() const;
        qint64 duration() const;
        qint64 position() const;
        PlayerState state() const;
        PlaybackMode playbackMode() const;
        AspectRatio aspectRatio() const;
        int volume() const;
        int brightness() const;
        int contrast() const;
        int hue() const;
        int saturation() const;
        bool isAudioMuted() const;
        bool isAudioAvailable() const;
        bool isVideoAvailable() const;
        bool isSeekable() const;
        bool isFullScreen() const;
        bool eventFilter(QObject *object, QEvent *event);

    public Q_SLOTS:
        void seekBackward();
        void seekForward();
        void increaseVolume();
        void decreaseVolume();
        void openDisc(const QString &device, PlaylistSource type);
        void play();
        void play(int index);
        void playPause();
        void pause();
        void stop();
        void playNext();
        void playPrevious();
        void setPlaylist(PlaylistModel *playlist);
        void setPosition(qint64 position);
        void setVolume(int volume);
        void setAudioMuted(bool muted);
        void setPlaybackMode(PlaybackMode mode);
        void setAspectRatio(AspectRatio ratio);
        void setVideoMode(bool mode, bool force = false);
        void setFullScreen(bool enable);
        void setInhibitNotifications(bool inhibit);
        void setBrightness(int value);
        void setContrast(int value);
        void setHue(int value);
        void setSaturation(int value);

    protected:
        void timerEvent(QTimerEvent *event);
        void handleBusMessage(const QGst::MessagePtr &message);
        void handleVideoChange();
        void stateChanged(QGst::State state);
        PlayerState translateState(QGst::State state) const;

    protected Q_SLOTS:
        void volumeChanged(qreal volume = -1);
        void mediaChanged();
        void availableChaptersChanged();
        void availableAudioChannelsChanged();
        void availableSubtitlesChanged();
        void availableAnglesChanged();
        void availableTitlesChanged();
        void currentTrackChanged(int track, PlayerReaction reaction = NoReaction);
        void changeAspectRatio(QAction *action);
        void changeChapter(QAction *action);
        void changeAudioChannel(QAction *action);
        void changeSubtitles(QAction *action);
        void changeAngle(QAction *action);
        void updateSliders();
        void updateVideo();
        void updateVideoView();
        void updateLabel();
        void setUrl(const KUrl &url);

    private:
        QGst::PipelinePtr m_pipeline;
        KNotificationRestrictions *m_notificationRestrictions;
        VideoWidget *m_currentVideoWidget;
        VideoWidget *m_appletVideoWidget;
        VideoWidget *m_dialogVideoWidget;
        VideoWidget *m_fullScreenVideoWidget;
        QWidget *m_fullScreenWidget;
        QSlider *m_brightnessSlider;
        QSlider *m_contrastSlider;
        QSlider *m_hueSlider;
        QSlider *m_saturationSlider;
        QActionGroup *m_chaptersGroup;
        QActionGroup *m_audioChannelGroup;
        QActionGroup *m_subtitlesGroup;
        QActionGroup *m_anglesGroup;
        QPointer<PlaylistModel> m_playlist;
        QMap<PlayerAction, QAction*> m_actions;
        QMap<MetaDataKey, QString> m_metaData;
        KUrl m_url;
        AspectRatio m_aspectRatio;
        qint64 m_restartPosition;
        int m_hideFullScreenControlsTimer;
        int m_setupFullScreenTimer;
        int m_playPauseTimer;
        int m_stopSleepCookie;
        bool m_inhibitNotifications;
        bool m_ignoreMove;
        bool m_videoMode;
        Ui::fullScreen m_fullScreenUi;

    Q_SIGNALS:
        void modified();
        void metaDataChanged();
        void playlistChanged();
        void currentTrackChanged();
        void trackAdded(int track);
        void trackRemoved(int track);
        void trackChanged(int track);
        void durationChanged(qint64 duration);
        void positionChanged(qint64 position);
        void volumeChanged(int volume);
        void audioMutedChanged(bool muted);
        void audioAvailableChanged(bool available);
        void videoAvailableChanged(bool available);
        void fullScreenChanged(bool enabled);
        void seekableChanged(bool seekable);
        void stateChanged(PlayerState state);
        void playbackModeChanged(PlaybackMode mode);
        void errorOccured(QString error);
        void requestMenu(QPoint position);
        void requestDevicePlaylist(QString udi, KUrl::List tracks);
};

}

#endif
