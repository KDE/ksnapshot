bin_PROGRAMS= ksnapshot

INCLUDES = -DKSNAPVERSION="\"0.4.0\"" $(all_includes) 

ksnapshot_LDFLAGS = $(all_libraries) $(KDE_RPATH)
ksnapshot_LDADD = $(LIB_KIMGIO) $(LIB_KFILE)

ksnapshot_SOURCES = main.cpp ksnapshot.cpp preview.cpp

ksnapshot_METASOURCES = USE_AUTOMOC

lnkdir = $(kde_appsdir)/Graphics
lnk_DATA = ksnapshot.desktop

KDE_ICON = ksnapshot

EXTRA_DIST = $(lnk_DATA) 

messages:
	$(XGETTEXT) $(ksnapshot_SOURCES) -o $(podir)/ksnapshot.pot

