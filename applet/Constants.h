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

#ifndef MINIPLAYERCONSTANTS_HEADER
#define MINIPLAYERCONSTANTS_HEADER

namespace MiniPlayer
{

enum PlayerAction { OpenMenuAction, OpenFileAction, OpenUrlAction, PlayPauseAction, StopAction, NavigationMenuAction, ChapterMenuAction, PlayNextAction, PlayPreviousAction, SeekBackwardAction, SeekForwardAction, SeekToAction, VolumeAction, AudioMenuAction, AudioChannelMenuAction, IncreaseVolumeAction, DecreaseVolumeAction, MuteAction, VideoMenuAction, VideoPropepertiesMenu, AspectRatioMenuAction, SubtitleMenuAction, AngleMenuAction, FullScreenAction, PlaybackModeMenuAction };
enum PlayerState { PlayingState, PausedState, StoppedState, ErrorState };
enum PlaybackMode { SequentialMode = 0, LoopTrackMode = 1, LoopPlaylistMode = 2, RandomMode = 3 };
enum AspectRatio { AutomaticRatio = 0, Ratio4_3 = 1, Ratio16_9 = 2, FitToRatio = 3 };
enum PlaylistType { None = 0, PLS, M3U, XSPF, ASX };
enum PlaylistSource { Local = 0, Cd, Vcd, Dvd };

}

#endif
