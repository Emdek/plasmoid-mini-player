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

#ifndef MINIPLAYERPLAYER_HEADER
#define MINIPLAYERPLAYER_HEADER

#include <QtCore/QObject>
#include <QtGui/QAction>
#include <QtGui/QSlider>
#include <QtGui/QActionGroup>

#include <KUrl>
#include <KNotificationRestrictions>

#include <Phonon/AudioOutput>
#include <Phonon/MediaObject>
#include <Phonon/MediaSource>
#include <Phonon/VideoWidget>
#include <Phonon/MediaController>

#include "Constants.h"

namespace MiniPlayer
{

class PlaylistModel;
class VideoWidget;

class Player : public QObject
{
    Q_OBJECT

    public:
        Player(QObject *parent);

        void registerAppletVideoWidget(VideoWidget *videoWidget);
        void registerDialogVideoWidget(VideoWidget *videoWidget);
        void registerFullScreenVideoWidget(QWidget *videoWidget);
        QString errorString() const;
        QString title() const;
        PlaylistModel* playlist() const;
        QAction* action(PlayerAction action) const;
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
        bool eventFilter(QObject *object, QEvent *event);

    public slots:
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
        void setAspectRatio(AspectRatio ratio);
        void setVideoMode(bool mode);
        void setFullScreen(bool enable);
        void setBrightness(int value);
        void setContrast(int value);
        void setHue(int value);
        void setSaturation(int value);

    protected:
        PlayerState translateState(Phonon::State state) const;

    protected slots:
        void volumeChanged(qreal volume = -1);
        void videoChanged();
        void mediaChanged();
        void availableChaptersChanged();
        void availableAudioChannelsChanged();
        void availableSubtitlesChanged();
        void availableAnglesChanged();
        void availableTitlesChanged();
        void currentTrackChanged(int track, PlayerReaction reaction = NoReaction);
        void stateChanged(Phonon::State state);
        void changePlaybackMode(QAction *action);
        void changeAspectRatio(QAction *action);
        void changeChapter(QAction *action);
        void changeAudioChannel(QAction *action);
        void changeSubtitles(QAction *action);
        void changeAngle(QAction *action);
        void trackFinished();
        void updateSliders();

    private:
        Phonon::MediaObject *m_mediaObject;
        Phonon::MediaController *m_mediaController;
        Phonon::AudioOutput *m_audioOutput;
        Phonon::VideoWidget *m_videoWidget;
        PlaylistModel *m_playlist;
        KNotificationRestrictions *m_notificationRestrictions;
        VideoWidget *m_appletVideoWidget;
        VideoWidget *m_dialogVideoWidget;
        QWidget *m_fullScreenVideoWidget;
        QSlider *m_brightnessSlider;
        QSlider *m_contrastSlider;
        QSlider *m_hueSlider;
        QSlider *m_saturationSlider;
        QActionGroup *m_chaptersGroup;
        QActionGroup *m_audioChannelGroup;
        QActionGroup *m_subtitlesGroup;
        QActionGroup *m_anglesGroup;
        QHash<PlayerAction, QAction*> m_actions;
        AspectRatio m_aspectRatio;
        int m_stopSleepCookie;
        bool m_videoMode;
        bool m_fullScreenMode;

    signals:
        void needsSaving();
        void metaDataChanged();
        void trackChanged();
        void durationChanged(qint64 duration);
        void volumeChanged(int volume);
        void audioMutedChanged(bool muted);
        void audioAvailableChanged(bool available);
        void videoAvailableChanged(bool available);
        void seekableChanged(bool seekable);
        void stateChanged(PlayerState state);
        void errorOccured(QString error);
        void requestMenu(QPoint position);
        void createDevicePlaylist(QString udi, KUrl::List tracks);
};

}

#endif
