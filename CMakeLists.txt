project(ksnapshot)

configure_file(config-ksnapshot.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-ksnapshot.h)

########### next target ###############

set(ksnapshot_SRCS
   main.cpp
   ksnapshot.cpp
   regiongrabber.cpp
   windowgrabber.cpp
   openwith.cpp )

qt4_add_dbus_adaptor(ksnapshot_SRCS org.kde.ksnapshot.xml ksnapshot.h KSnapshot)

kde4_automoc(${ksnapshot_SRCS})

kde4_add_ui_files(ksnapshot_SRCS ksnapshotwidget.ui)

kde4_add_executable(ksnapshot ${ksnapshot_SRCS})

target_link_libraries(ksnapshot ${KDE4_KDECORE_LIBS})

install(TARGETS ksnapshot DESTINATION ${BIN_INSTALL_DIR})

########### install files ###############

install(FILES ksnapshot.desktop DESTINATION ${XDG_APPS_DIR})

kde4_install_icons(${ICON_INSTALL_DIR} )
