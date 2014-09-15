project(ksnapshot)

cmake_minimum_required (VERSION 2.8.12 FATAL_ERROR)
set (QT_MIN_VERSION "5.2.0")

#macro_optional_find_package(Kipi)

find_package(ECM 1.1.0 REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
                      ${CMAKE_MODULE_PATH}
                      ${ECM_MODULE_PATH}
                      ${ECM_KDE_MODULE_DIR})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(ECMOptionalAddSubdirectory)
include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMMarkNonGuiExecutable)
include(ECMGenerateHeaders)
include(GenerateExportHeader)
include(FeatureSummary)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED Core DBus Widgets X11Extras)

find_package(KF5 REQUIRED
       Config ConfigWidgets
       CoreAddons DocTools GuiAddons I18n
       KDELibs4Support KIO
       Service WindowSystem
       XmlGui
)

add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

find_package(X11)
if (X11_FOUND)
    set(HAVE_X11 1)
endif(X11_FOUND)

find_package(XCB MODULE COMPONENTS XCB SHAPE XFIXES)
set_package_properties(XCB PROPERTIES DESCRIPTION "X protocol C-language Binding"
                       URL "http://xcb.freedesktop.org"
                       TYPE OPTIONAL)
find_package(X11_XCB MODULE)
set_package_properties(X11_XCB PROPERTIES DESCRIPTION "Xlib/XCB interface library"
                       URL "http://xcb.freedesktop.org"
                       TYPE OPTIONAL)

#include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if (KIPI_FOUND)
   include_directories(${KIPI_INCLUDE_DIR})
endif (KIPI_FOUND)

configure_file(config-ksnapshot.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-ksnapshot.h)

if (XCB_XCB_FOUND AND X11_XCB_FOUND)
   set(ksnapshot_xcb_SRCS
       xcb/pixmaphelper.cpp)
endif (XCB_XCB_FOUND AND X11_XCB_FOUND)

########### next target ###############

add_subdirectory( doc )

set(ksnapshot_file_SRCS
   expblur.cpp
   regiongrabber.cpp
   freeregiongrabber.cpp
   snapshottimer.cpp
   windowgrabber.cpp
   ksnapshotobject.cpp
   ksnapshotpreview.cpp)

if (KIPI_FOUND)
    set(ksnapshot_kipi_SRCS
        kipiinterface.cpp
        ksnapshotimagecollectionshared.cpp
        ksnapshotinfoshared.cpp
        kipiimagecollectionselector.cpp)
endif (KIPI_FOUND)

set(ksnapshot_SRCS
   main.cpp
   ksnapshot.cpp 
   ${ksnapshot_xcb_SRCS}
   ${ksnapshot_file_SRCS}
   ${ksnapshot_kipi_SRCS})

qt5_add_dbus_adaptor(ksnapshot_SRCS org.kde.ksnapshot.xml ksnapshot.h KSnapshot)

ki18n_wrap_ui(ksnapshot_SRCS ksnapshotwidget.ui)

ecm_install_icons(ICONS 
   icons/16-apps-ksnapshot.png
   icons/22-apps-ksnapshot.png
   icons/32-apps-ksnapshot.png
   icons/48-apps-ksnapshot.png
   DESTINATION ${ICON_INSTALL_DIR} THEME hicolor)

add_executable(ksnapshot ${ksnapshot_SRCS})

target_link_libraries(ksnapshot
   Qt5::X11Extras
   KF5::CoreAddons
   KF5::ConfigGui
   KF5::I18n
   KF5::KIOWidgets
   KF5::WindowSystem
   KF5::XmlGui
   ${X11_LIBRARIES}
   )

if (X11_XCB_FOUND)
    target_link_libraries(ksnapshot ${X11_XCB_LIBRARIES})
endif ()

if (XCB_XCB_FOUND)
    target_link_libraries(ksnapshot ${XCB_XCB_LIBRARY})
endif ()

if (XCB_SHAPE_FOUND)
    target_link_libraries(ksnapshot ${XCB_SHAPE_LIBRARY})
endif ()

if (XCB_XFIXES_FOUND)
    target_link_libraries(ksnapshot ${XCB_XFIXES_LIBRARY})
endif ()

if (KIPI_FOUND)
    target_link_libraries(ksnapshot ${KIPI_LIBRARIES})
endif (KIPI_FOUND)

install(TARGETS ksnapshot ${INSTALL_TARGETS_DEFAULT_ARGS})


########### next target ###############

set(kbackgroundsnapshot_SRCS
   kbackgroundsnapshot.cpp
   )

add_executable(kbackgroundsnapshot
               ${kbackgroundsnapshot_SRCS}
               ${ksnapshot_xcb_SRCS}
               ${ksnapshot_file_SRCS}
               )

target_link_libraries(kbackgroundsnapshot
                      Qt5::X11Extras
                      KF5::CoreAddons
                      KF5::ConfigGui
                      KF5::I18n
                      KF5::KIOWidgets
                      KF5::WindowSystem
                      ${X11_LIBRARIES}
                     )

target_link_libraries(kbackgroundsnapshot ${KDE4_KIO_LIBS})

if (X11_XCB_FOUND)
    target_link_libraries(kbackgroundsnapshot ${X11_XCB_LIBRARIES})
endif ()

if (XCB_XCB_FOUND)
    target_link_libraries(kbackgroundsnapshot ${XCB_XCB_LIBRARY})
endif ()

if (XCB_SHAPE_FOUND)
    target_link_libraries(kbackgroundsnapshot ${XCB_SHAPE_LIBRARY})
endif ()

if (XCB_XFIXES_FOUND)
    target_link_libraries(kbackgroundsnapshot ${XCB_XFIXES_LIBRARY})
endif ()

install(TARGETS kbackgroundsnapshot ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############


install(PROGRAMS ksnapshot.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES org.kde.ksnapshot.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
