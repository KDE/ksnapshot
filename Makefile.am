bin_PROGRAMS= ksnapshot

INCLUDES = -DKSNAPVERSION="\"0.5\"" $(all_includes)

ksnapshot_LDFLAGS = $(all_libraries) $(KDE_RPATH)
ksnapshot_LDADD = $(LIB_KSYCOCA) $(LIB_KFILE) -lkdeprint

ksnapshot_SOURCES = ksnapshotiface.skel main.cpp ksnapshot.cpp ksnapshotbase.ui

ksnapshot_METASOURCES = USE_AUTOMOC

lnkdir = $(kde_appsdir)/Graphics
lnk_DATA = ksnapshot.desktop

KDE_ICON = ksnapshot

EXTRA_DIST = $(lnk_DATA)

messages: rc.cpp
	$(XGETTEXT) rc.cpp *.cpp -o $(podir)/ksnapshot.pot

main.cpp: ksnapshotbase.h

