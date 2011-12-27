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

#ifndef MINIPLAYERPLAYLISTMANAGER_HEADER
#define MINIPLAYERPLAYLISTMANAGER_HEADER

#include <QtCore/QObject>

#include <Plasma/Dialog>

#include "ui_playlist.h"

namespace MiniPlayer
{

enum PlaylistType { None = 0, PLS, M3U, XSPF, ASX };

class Player;
class PlaylistModel;
class VideoWidget;

class PlaylistManager : public QObject
{
    Q_OBJECT

    public:
        PlaylistManager(Player *parent);

        void addTracks(const KUrl::List &tracks, int index = -1, bool play = false);
        QList<PlaylistModel*> playlists() const;
        QSize dialogSize() const;
        QByteArray splitterState() const;
        QByteArray headerState() const;
        int currentPlaylist() const;
        int visiblePlaylist() const;
        bool isDialogVisible() const;
        bool eventFilter(QObject *object, QEvent *event);

    public slots:
        void createPlaylist(const QString &playlist, const KUrl::List &tracks = KUrl::List());
        void filterPlaylist(const QString &text);
        void movePlaylist(int from, int to);
        void renamePlaylist(int position);
        void removePlaylist(int position);
        void visiblePlaylistChanged(int position);
        void exportPlaylist();
        void newPlaylist();
        void clearPlaylist();
        void shufflePlaylist();
        void setCurrentPlaylist(int position);
        void setDialogSize(const QSize &size);
        void setSplitterState(const QByteArray &state);
        void setHeaderState(const QByteArray &state);
        void showDialog(const QPoint &position);
        void closeDialog();

    public slots:
        void trackPressed();
        void trackChanged();
        void moveUpTrack();
        void moveDownTrack();
        void playTrack(QModelIndex index = QModelIndex());
        void editTrackTitle();
        void copyTrackUrl();
        void removeTrack();

    private:
        Player *m_player;
        Plasma::Dialog *m_dialog;
        VideoWidget *m_videoWidget;
        QList<PlaylistModel*> m_playlists;
        int m_currentPlaylist;
        bool m_editorActive;
        Ui::playlist m_playlistUi;

    signals:
        void configNeedsSaving();
};

}

#endif
