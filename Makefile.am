bin_PROGRAMS= ksnapshot

INCLUDES = -DKSNAPVERSION="\"0.2.7\"" $(all_includes) 

ksnapshot_LDFLAGS = $(all_libraries) $(KDE_RPATH)
ksnapshot_LDADD = $(LIB_KIMGIO) $(LIB_KDEUI)

ksnapshot_SOURCES = eps.cpp gif.cpp main.cpp formats.cpp ksnapshot.cpp preview.cpp

ksnapshot_METASOURCES = USE_AUTOMOC

lnkdir = $(kde_appsdir)/Graphics
lnk_DATA = ksnapshot.kdelnk

icondir = $(kde_icondir)
icon_DATA = ksnapshot.xpm

EXTRA_DIST = $(lnk_DATA) $(icon_DATA) mini-kpaint.xpm

install-data-local: mini-ksnapshot.xpm
	$(mkinstalldirs) $(kde_minidir)
	$(INSTALL_DATA) $(srcdir)/mini-ksnapshot.xpm $(kde_minidir)/ksnapshot.xpm

uninstall-local:
	-rm -f $(kde_minidir)/ksnapshot.xpm

messages:
	$(XGETTEXT) -C -ktranslate -ki18n -x $(includedir)/kde.pot $(ksnapshot_SOURCES) && mv messages.po ../po/ksnapshot.pot

