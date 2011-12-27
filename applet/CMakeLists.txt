project(miniplayer)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(
   ${CMAKE_SOURCE_DIR}
   ${CMAKE_BINARY_DIR}
   ${KDE4_INCLUDES}
   )

set(miniplayer_SRCS Applet.cpp Player.cpp MetaDataManager.cpp PlaylistManager.cpp PlaylistModel.cpp PlaylistReader.cpp VideoWidget.cpp SeekSlider.cpp VolumeSlider.cpp PlayerDBusHandler.cpp RootDBusHandler.cpp TrackListDBusHandler.cpp)

add_subdirectory(locale)

QT4_ADD_DBUS_ADAPTOR(miniplayer_SRCS dbus/org.freedesktop.MediaPlayer.player.xml PlayerDBusHandler.h PlayerDBusHandler PlayerAdaptor PlayerAdaptor)
QT4_ADD_DBUS_ADAPTOR(miniplayer_SRCS dbus/org.freedesktop.MediaPlayer.root.xml RootDBusHandler.h RootDBusHandler RootAdaptor RootAdaptor)
QT4_ADD_DBUS_ADAPTOR(miniplayer_SRCS dbus/org.freedesktop.MediaPlayer.tracklist.xml TrackListDBusHandler.h TrackListDBusHandler TrackListAdaptor TrackListAdaptor)

kde4_add_ui_files(miniplayer_SRCS ui/general.ui ui/controls.ui ui/jumpToPosition.ui ui/playlist.ui ui/fullScreen.ui ui/volume.ui)
kde4_add_plugin(plasma_applet_miniplayer ${miniplayer_SRCS})
target_link_libraries(plasma_applet_miniplayer
                      ${QT_QTDBUS_LIBRARY}
                      ${KDE4_PLASMA_LIBS}
                      ${KDE4_PHONON_LIBS}
                      ${KDE4_SOLID_LIBS}
                      ${KDE4_KDEUI_LIBS}
                      ${KDE4_KIO_LIBS}
                      )

install(TARGETS plasma_applet_miniplayer
        DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES plasma-applet-miniplayer.desktop
        DESTINATION ${SERVICES_INSTALL_DIR})
