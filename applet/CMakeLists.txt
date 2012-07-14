project(miniplayer)

find_package(QtGStreamer REQUIRED)
find_package(KDE4 REQUIRED)
include(KDE4Defaults)

add_definitions(${QT_DEFINITIONS} ${QTGSTREAMER_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${QTGSTREAMER_INCLUDES} ${KDE4_INCLUDES})

set(miniplayer_SRCS Applet.cpp Configuration.cpp Player.cpp MetaDataManager.cpp PlaylistManager.cpp PlaylistModel.cpp PlaylistReader.cpp VideoWidget.cpp SeekSlider.cpp VolumeSlider.cpp DBusInterface.cpp DBusRootAdaptor.cpp DBusTrackListAdaptor.cpp DBusPlayerAdaptor.cpp DBusPlaylistsAdaptor.cpp)
set(CMAKE_CXX_FLAGS ${QTGSTREAMER_FLAGS})

add_subdirectory(locale)

kde4_add_ui_files(miniplayer_SRCS ui/general.ui ui/controls.ui ui/jumpToPosition.ui ui/playlist.ui ui/track.ui ui/fullScreen.ui ui/volume.ui)
kde4_add_plugin(plasma_applet_miniplayer ${miniplayer_SRCS})

target_link_libraries(plasma_applet_miniplayer
	${QT_QTDBUS_LIBRARY}
	${QTGSTREAMER_UI_LIBRARIES}
	${KDE4_PLASMA_LIBS}
	${KDE4_SOLID_LIBS}
	${KDE4_KDEUI_LIBS}
	${KDE4_KIO_LIBS}
	)

install(TARGETS plasma_applet_miniplayer
	DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES plasma-applet-miniplayer.desktop
	DESTINATION ${SERVICES_INSTALL_DIR})
