bin_PROGRAMS= ksnapshot

INCLUDES = -DKSNAPVERSION="\"0.7\"" $(all_includes)

ksnapshot_LDFLAGS = $(all_libraries) $(KDE_RPATH)
ksnapshot_LDADD = $(LIB_KDEPRINT)

ksnapshot_SOURCES = ksnapshotiface.skel main.cpp ksnapshot.cpp \
    regiongrabber.cpp windowgrabber.cpp ksnapshotwidget.ui

ksnapshot_METASOURCES = AUTO

xdg_apps_DATA = ksnapshot.desktop

KDE_ICON = ksnapshot

EXTRA_DIST = $(xdg_apps_DATA)

messages: rc.cpp
	$(XGETTEXT) rc.cpp *.cpp -o $(podir)/ksnapshot.pot

