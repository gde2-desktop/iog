AUTOMAKE_OPTIONS = subdir-objects

pluginsdir = $(libdir)/iog/plugins

AM_CPPFLAGS = \
	-I$(top_srcdir)/src 				\
	$(IOG_CFLAGS) 					\
	$(WARN_CFLAGS)					\
	-DIOG_LOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

AM_LDFLAGS = -avoid-version -module

plugins_LTLIBRARIES = \
	fullscreen/libfullscreen.la  \
	reload/libreload.la

plugins_in_files = \
	fullscreen/fullscreen.iog-plugin.desktop.in  \
	reload/reload.iog-plugin.desktop.in

# Fullscreen plugin
fullscreen_libfullscreen_la_SOURCES = \
	fullscreen/iog-fullscreen-plugin.h  \
	fullscreen/iog-fullscreen-plugin.c

fullscreen_libfullscreen_la_LIBADD = $(IOG_LIBS)

# Reload plugin
reload_libreload_la_SOURCES = \
	reload/iog-reload-plugin.h  \
	reload/iog-reload-plugin.c

reload_libreload_la_LIBADD = $(IOG_LIBS)

# Statusbar Date Plugin
if HAVE_EXIF
plugins_LTLIBRARIES += statusbar-date/libstatusbar-date.la

statusbar_date_libstatusbar_date_la_SOURCES = \
	statusbar-date/iog-statusbar-date-plugin.h  \
	statusbar-date/iog-statusbar-date-plugin.c

statusbar_date_libstatusbar_date_la_LIBADD = $(IOG_LIBS)
plugins_in_files += statusbar-date/statusbar-date.iog-plugin.desktop.in
endif

# Generate plugin info files

%.iog-plugin: %.iog-plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(AM_V_GEN)$(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugins_DATA = $(plugins_in_files:.iog-plugin.desktop.in=.iog-plugin)

DISTCLEANFILES = $(plugins_DATA)
CLEANFILES = $(plugins_DATA)

EXTRA_DIST = $(plugins_in_files)

-include $(top_srcdir)/git.mk
