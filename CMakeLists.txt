project(ksnapshot)

cmake_minimum_required (VERSION 2.8.12 FATAL_ERROR)
set (QT_MIN_VERSION "5.2.0")

#macro_optional_find_package(Kipi)

find_package(ECM 1.1.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

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

    if (X11_Xshape_FOUND)
        set(HAVE_X11_EXTENSIONS_SHAPE_H 1)
    endif (X11_Xshape_FOUND)

    if (X11_Xfixes_FOUND)
        set(HAVE_X11_EXTENSIONS_XFIXES_H 1)
    endif (X11_Xfixes_FOUND)
endif (X11_FOUND)
#feature_summary(WHAT ENABLED_FEATURES DESCRIPTION "X11 Fixes Extension (xfixes.h)" "Support for capturing the cursor" "http://www.x.org/" VAR HAVE_X11_EXTENSIONS_XFIXES_H)

if (KIPI_FOUND)
        include_directories(${KIPI_INCLUDE_DIR})
endif (KIPI_FOUND)
#macro_log_feature(KIPI_FOUND "KIPI plugins" "Provides various image export features, such as printing, emailing and uploading" "http://www.kipi-plugins.org/" FALSE "" "")

configure_file(config-ksnapshot.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-ksnapshot.h)

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
   ${X11_LIBRARIES})

if (X11_Xfixes_FOUND)
    target_link_libraries(ksnapshot ${X11_Xfixes_LIB})
endif (X11_Xfixes_FOUND)

if (KIPI_FOUND)
    target_link_libraries(ksnapshot ${KIPI_LIBRARIES})
endif (KIPI_FOUND)

install(TARGETS ksnapshot ${INSTALL_TARGETS_DEFAULT_ARGS})


########### next target ###############

#set(kbackgroundsnapshot_SRCS
#   kbackgroundsnapshot.cpp
#   ${ksnapshot_file_SRCS})


#add_executable(kbackgroundsnapshot ${kbackgroundsnapshot_SRCS})

#target_link_libraries(kbackgroundsnapshot KF5::KIOWidgets ${X11_LIBRARIES})

#install(TARGETS kbackgroundsnapshot ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############


install(PROGRAMS ksnapshot.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES org.kde.ksnapshot.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
