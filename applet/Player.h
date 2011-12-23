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

#ifndef MINIPLAYERPLYER_HEADER
#define MINIPLAYERPLYER_HEADER

#include <QtCore/QObject>
#include <QtGui/QAction>
#include <QtMultimediaKit/QMediaPlayer>
#include <QtMultimediaKit/QMediaPlaylist>

namespace MiniPlayer
{

enum PlayerAction { OpenMenuAction, OpenFileAction, OpenUrlAction, PlayPauseAction, StopAction, VolumeAction, AudioMenuAction, MuteAction, IncreaseVolumeAction, DecreaseVolumeAction, NavigationMenuAction, PlayNextAction, PlayPreviousAction, JumpBackAction, JumpForwardAction, JumpToAction, VideoMenuAction };
enum PlayerState { PlayingState, PausedState, StoppedState, ErrorState };

class Player : public QObject
{
    Q_OBJECT

    public:
        Player(QObject *parent = 0);

        QString errorString() const;
        QString title() const;
        QMediaPlaylist* playlist() const;
        QAction* action(PlayerAction action) const;
        qint64 duration() const;
        qint64 position() const;
        PlayerState state() const;
        int volume() const;
        bool isAudioMuted() const;
        bool isAudioAvailable() const;
        bool isVideoAvailable() const;
        bool isSeekable() const;

    public slots:
        void seekBackward();
        void seekForward();
        void increaseVolume();
        void decreaseVolume();
        void play();
        void playPause();
        void pause();
        void stop();
        void setPlaylist(QMediaPlaylist *playlist);
        void setPosition(qint64 position);
        void setVolume(int volume);
        void setAudioMuted(bool muted);

    protected:
        PlayerState translateState(QMediaPlayer::State state) const;

    protected slots:
        void stateChanged(QMediaPlayer::State state);
        void errorOccured(QMediaPlayer::Error error);

    private:
        QMediaPlayer *m_mediaPlayer;
        QHash<PlayerAction, QAction*> m_actions;

    signals:
        void metaDataChanged();
        void durationChanged(qint64 duration);
        void volumeChanged(int volume);
        void audioMutedChanged(bool muted);
        void audioAvailableChanged(bool available);
        void videoAvailableChanged(bool available);
        void seekableChanged(bool seekable);
        void stateChanged(PlayerState state);
        void errorOccured(QString error);
};

}

#endif
