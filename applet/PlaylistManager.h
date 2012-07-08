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
#include "ui_track.h"

namespace MiniPlayer
{

class Player;
class PlaylistModel;
class VideoWidget;

class PlaylistManager : public QObject
{
    Q_OBJECT

    public:
        explicit PlaylistManager(Player *parent);

        void addTracks(const KUrl::List &tracks, int index = -1, PlayerReaction reaction = NoReaction);
        void removeTracks(const KUrl::List &tracks);
        PlaylistModel* playlist(int id) const;
        QList<int> playlists() const;
        QStringList columnsOrder() const;
        QStringList columnsVisibility() const;
        QSize dialogSize() const;
        QByteArray splitterState() const;
        QByteArray headerState() const;
        PlayerState state() const;
        int createPlaylist(const QString &playlist, const KUrl::List &tracks = KUrl::List(), PlaylistSource source = LocalSource, int id = -1);
        int currentPlaylist() const;
        int visiblePlaylist() const;
        bool isDialogVisible() const;
        bool isSplitterLocked() const;
        bool eventFilter(QObject *object, QEvent *event);

    public slots:
        void showDialog(const QPoint &position);
        void closeDialog();
        void setCurrentPlaylist(int id);
        void setDialogSize(const QSize &size);
        void setPlaylistsOrder(const QList<int> &order);
        void setColumnsOrder(const QStringList &order);
        void setColumnsVisibility(const QStringList &visibility);
        void setSplitterLocked(bool locked);
        void setSplitterState(const QByteArray &state);
        void setHeaderState(const QByteArray &state);

    protected:
        void timerEvent(QTimerEvent *event);

    protected slots:
        void columnsOrderChanged();
        void visiblePlaylistChanged(int position);
        void playbackModeChanged(QAction *action);
        void toggleColumnVisibility(QAction *action);
        void openDisc(QAction *action);
        void deviceAdded(const QString &udi);
        void deviceRemoved(const QString &udi);
        void createDevicePlaylist(const QString &udi, const KUrl::List &tracks);
        void playlistMoved(int from, int to);
        void filterPlaylist();
        void filterPlaylist(const QString &text);
        void renamePlaylist(int id = -1);
        void removePlaylist(int id = -1);
        void exportPlaylist();
        void newPlaylist();
        void clearPlaylist();
        void shufflePlaylist();
        void trackChanged();
        void moveUpTrack();
        void moveDownTrack();
        void removeTrack();
        void playTrack(QModelIndex index = QModelIndex());
        void editTrack(QAction *action = NULL);
        void copyTrack(QAction *action);
        void saveTrack();
        void copyTrackUrl();
        void updateActions();
        void updateTheme();
        void updateLabel();
        void updateVideoView();

    private:
        Player *m_player;
        Plasma::Dialog *m_dialog;
        VideoWidget *m_videoWidget;
        QMap<int, PlaylistModel*> m_playlists;
        QMap<PlaylistColumn, QString> m_columns;
        QMap<QString, QPair<QAction*, QMap<QString, QVariant> > > m_discActions;
        QSet<KUrl> m_removedTracks;
        QList<int> m_playlistsOrder;
        QStringList m_columnsOrder;
        QStringList m_columnsVisibility;
        QSize m_size;
        QByteArray m_splitterState;
        QByteArray m_headerState;
        int m_selectedPlaylist;
        int m_removeTracks;
        bool m_splitterLocked;
        bool m_isEdited;
        Ui::playlist m_playlistUi;
        Ui::track m_trackUi;

    signals:
        void playlistAdded(int id);
        void playlistRemoved(int id);
        void currentPlaylistChanged(int id);
        void playlistChanged(int id);
        void modified();
        void requestMenu(QPoint position);
};

}

#endif
