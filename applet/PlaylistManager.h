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

#ifndef MINIPLAYERPLAYLISTMANAGER_HEADER
#define MINIPLAYERPLAYLISTMANAGER_HEADER

#include <Plasma/Dialog>

#include "Constants.h"

#include "ui_playlist.h"

namespace MiniPlayer
{

class Player;
class PlaylistModel;
class VideoWidget;

class PlaylistManager : public QObject
{
    Q_OBJECT

    public:
        PlaylistManager(Player *parent);

        void addTracks(const KUrl::List &tracks, int index = -1, PlayerReaction reaction = NoReaction);
        void removeTracks(const KUrl::List &tracks);
        QList<PlaylistModel*> playlists() const;
        QList<int> sectionsOrder() const;
        QSize dialogSize() const;
        QByteArray splitterState() const;
        QByteArray headerState() const;
        PlayerState state() const;
        int createPlaylist(const QString &playlist, const KUrl::List &tracks = KUrl::List(), PlaylistSource source = LocalSource);
        int currentPlaylist() const;
        int visiblePlaylist() const;
        bool isDialogVisible() const;
        bool isSplitterLocked() const;
        bool eventFilter(QObject *object, QEvent *event);

    public slots:
        void showDialog(const QPoint &position);
        void closeDialog();
        void setCurrentPlaylist(int position);
        void setDialogSize(const QSize &size);
        void setSectionsOrder(const QList<int> &order);
        void setSplitterLocked(bool locked);
        void setSplitterState(const QByteArray &state);
        void setHeaderState(const QByteArray &state);

    protected:
        void timerEvent(QTimerEvent *event);

    protected slots:
        void sectionsOrderChanged();
        void visiblePlaylistChanged(int position);
        void playbackModeChanged(QAction *action);
        void openDisc(QAction *action);
        void deviceAdded(const QString &udi);
        void deviceRemoved(const QString &udi);
        void createDevicePlaylist(const QString &udi, const KUrl::List &tracks);
        void playlistMoved(int from, int to);
        void filterPlaylist(const QString &text);
        void renamePlaylist(int position = -1);
        void removePlaylist(int position = -1);
        void exportPlaylist();
        void newPlaylist();
        void clearPlaylist();
        void shufflePlaylist();
        void trackChanged();
        void moveUpTrack();
        void moveDownTrack();
        void removeTrack();
        void playTrack(QModelIndex index = QModelIndex());
        void editTrack(QAction *action);
        void copyTrackUrl();
        void updateActions();
        void updateTheme();
        void updateVideoView();

    private:
        Player *m_player;
        Plasma::Dialog *m_dialog;
        VideoWidget *m_videoWidget;
        QHash<QString, QPair<QAction*, QHash<QString, QVariant> > > m_discActions;
        QList<PlaylistModel*> m_playlists;
        QList<int> m_sectionsOrder;
        QSet<KUrl> m_removedTracks;
        QSize m_size;
        QByteArray m_splitterState;
        QByteArray m_headerState;
        int m_selectedPlaylist;
        int m_removeTracks;
        bool m_splitterLocked;
        bool m_isEdited;
        Ui::playlist m_playlistUi;

    signals:
        void needsSaving();
        void requestMenu(QPoint position);
};

}

#endif
