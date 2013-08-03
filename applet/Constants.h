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

#ifndef MINIPLAYERCONSTANTS_HEADER
#define MINIPLAYERCONSTANTS_HEADER

namespace MiniPlayer
{

enum PlayerAction { OpenMenuAction, OpenFileAction, OpenUrlAction, PlayPauseAction, StopAction, VolumeToggleAction, PlaylistToggleAction, NavigationMenuAction, ChapterMenuAction, PlayNextAction, PlayPreviousAction, SeekBackwardAction, SeekForwardAction, SeekToAction, AudioMenuAction, AudioChannelMenuAction, IncreaseVolumeAction, DecreaseVolumeAction, MuteAction, VideoMenuAction, VideoPropepertiesMenu, AspectRatioMenuAction, SubtitleMenuAction, AngleMenuAction, FullScreenAction, PlaybackModeMenuAction };
enum PlayerState { PlayingState, PausedState, StoppedState, ErrorState };
enum PlayerReaction { NoReaction, PlayReaction, PauseReaction, StopReaction };
enum PlaybackMode { SequentialMode = 0, LoopTrackMode = 1, LoopPlaylistMode = 2, RandomMode = 3, CurrentTrackOnceMode = 4 };
enum AspectRatio { AutomaticRatio = 0, Ratio4_3 = 1, Ratio16_9 = 2, FitToRatio = 3 };
enum PlaylistFormat { InvalidFormat = 0, PlsFormat, M3uFormat, XspfFormat, AsxFormat };
enum PlaylistSource { LocalSource = 0, CdSource, VcdSource, DvdSource };
enum PlaylistColumn { FileTypeColumn = 0, FileNameColumn, ArtistColumn, TitleColumn, AlbumColumn, TrackNumberColumn, GenreColumn, DescriptionColumn, DateColumn, DurationColumn };
enum MetaDataKey { InvalidKey = 0, TitleKey = 1, ArtistKey = 2, AlbumKey = 4, DateKey = 8, GenreKey = 16, DescriptionKey = 32, TrackNumberKey = 64 };

}

#endif
