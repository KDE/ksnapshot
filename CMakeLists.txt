project(ksnapshot)




########### next target ###############

ADD_DEFINITIONS(-DKSNAPVERSION=\"\\"0.7\\"\")

set(ksnapshot_SRCS 
   main.cpp 
   ksnapshot.cpp 
   regiongrabber.cpp 
   windowgrabber.cpp )

kde4_automoc(${ksnapshot_SRCS})

kde4_add_ui3_files(ksnapshot_SRCS ksnapshotwidget.ui )

kde4_add_dcop_skels(ksnapshot_SRCS ksnapshotiface.h )

kde4_add_executable(ksnapshot ${ksnapshot_SRCS})

target_link_libraries(ksnapshot  ${KDE4_KDECORE_LIBS} kdeprint )

install_targets(/bin ksnapshot )


########### install files ###############

install_files( ${XDG_APPS_DIR} FILES ksnapshot.desktop )

kde4_install_icons( ${ICON_INSTALL_DIR}  hicolor )




#original Makefile.am contents follow:

#bin_PROGRAMS= ksnapshot
#
#INCLUDES = -DKSNAPVERSION="\"0.7\"" $(all_includes)
#
#ksnapshot_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#ksnapshot_LDADD = $(LIB_KDEPRINT)
#
#ksnapshot_SOURCES = ksnapshotiface.skel main.cpp ksnapshot.cpp \
#    regiongrabber.cpp windowgrabber.cpp ksnapshotwidget.ui
#
#ksnapshot_METASOURCES = AUTO
#
#xdg_apps_DATA = ksnapshot.desktop
#
#KDE_ICON = ksnapshot
#
#EXTRA_DIST = $(xdg_apps_DATA)
#
#messages: rc.cpp
#	$(XGETTEXT) rc.cpp *.cpp -o $(podir)/ksnapshot.pot
#
