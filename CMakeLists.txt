# KSnapshot project

project(KSnapshot)
set(KSnapshot_VERSION_MAJOR 1)
set(KSnapshot_VERSION_MINOR 0)
set(KSnapshot_VERSION_PATCH 0)
set(KSnapshot_VERSION "${KSnapshot_VERSION_MAJOR}.${KSnapshot_VERSION_MINOR}.${KSnapshot_VERSION_PATCH}")

# minimum requirements

cmake_minimum_required (VERSION 2.8.12 FATAL_ERROR)
set(QT_MIN_VERSION "5.2.0")
set(KF5_MIN_VERSION "5.6.0")

find_package(ECM 1.7.0 REQUIRED NO_MODULE)
set(
    CMAKE_MODULE_PATH
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
        ${CMAKE_MODULE_PATH}
        ${ECM_MODULE_PATH}
        ${ECM_KDE_MODULE_DIR}
)

# set up kf5

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
include(ECMAddAppIcon)

find_package(
    Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED
        Core
        DBus
        Widgets
        X11Extras
)

find_package(
    KF5 ${KF5_MIN_VERSION} REQUIRED
       Config
       ConfigWidgets
       CoreAddons
       DocTools
       GuiAddons
       I18n
       KIO
       Service
       WindowSystem
       XmlGui
       DBusAddons
       Parts
)

# optional components

find_package(KF5Kipi)
if (KF5Kipi_FOUND)
    set(KIPI_FOUND 1)
endif ()

find_package(X11)
if (X11_FOUND)
    set(HAVE_X11 1)
endif(X11_FOUND)

# set up xcb and x11_xcb

find_package(
    XCB MODULE COMPONENTS
        XCB
        SHAPE
        XFIXES
)

find_package(
    X11_XCB MODULE
)

set_package_properties(
    XCB
        PROPERTIES DESCRIPTION "X protocol C-language Binding"
        URL "http://xcb.freedesktop.org"
        TYPE OPTIONAL
)

set_package_properties(
    X11_XCB
        PROPERTIES DESCRIPTION "Xlib/XCB interface library"
        URL "http://xcb.freedesktop.org"
        TYPE OPTIONAL
)

# hand off to subdirectories

add_subdirectory(doc)
include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.Sources.txt)
include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.Icons.txt)
include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.Desktop.txt)

# summaries

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
