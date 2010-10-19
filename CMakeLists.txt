project(ksnapshot)

include(MacroBoolTo01)

set(HAVE_X11_EXTENSIONS_SHAPE_H X11_Xshape_FOUND)
set(HAVE_X11_EXTENSIONS_XFIXES_H X11_Xfixes_FOUND)
macro_log_feature(HAVE_X11_EXTENSIONS_XFIXES_H "X11 Fixes Extension (xfixes.h)" "X11 XFixes Extension" "http://www.x.org/" FALSE "" "Needed to capture the cursor in ksnapshot.")

configure_file(config-ksnapshot.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-ksnapshot.h)

########### next target ###############

set(ksnapshot_file_SRCS
   expblur.cpp
	regiongrabber.cpp
	snapshottimer.cpp
	windowgrabber.cpp
	ksnapshotobject.cpp
	ksnapshotpreview.cpp)

set(ksnapshot_SRCS
   main.cpp
   ksnapshot.cpp 
   ${ksnapshot_file_SRCS})

qt4_add_dbus_adaptor(ksnapshot_SRCS org.kde.ksnapshot.xml ksnapshot.h KSnapshot)


kde4_add_ui_files(ksnapshot_SRCS ksnapshotwidget.ui)

kde4_add_app_icon(ksnapshot_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/hi*-app-ksnapshot.png")

kde4_add_executable(ksnapshot ${ksnapshot_SRCS})

target_link_libraries(ksnapshot ${KDE4_KIO_LIBS} ${X11_LIBRARIES})

if (X11_Xfixes_FOUND)
    target_link_libraries(ksnapshot ${X11_Xfixes_LIB})
endif (X11_Xfixes_FOUND)

install(TARGETS ksnapshot ${INSTALL_TARGETS_DEFAULT_ARGS})


########### next target ###############

set(kbackgroundsnapshot_SRCS
   kbackgroundsnapshot.cpp
   ${ksnapshot_file_SRCS})


kde4_add_executable(kbackgroundsnapshot ${kbackgroundsnapshot_SRCS})

target_link_libraries(kbackgroundsnapshot ${KDE4_KIO_LIBS} ${X11_LIBRARIES})

install(TARGETS kbackgroundsnapshot ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############



install(PROGRAMS ksnapshot.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES org.kde.ksnapshot.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
kde4_install_icons(${ICON_INSTALL_DIR} )

