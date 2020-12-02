if ENABLE_JPEG
jpeg_LIB = $(top_builddir)/jpegutils/libiog-jpegutils.la
endif

toolbar_LIB = $(top_builddir)/cut-n-paste/toolbar-editor/libtoolbareditor.la

screensaver_LIB = $(top_builddir)/cut-n-paste/totem-screensaver/libtotemscrsaver.la

noinst_LTLIBRARIES = libiog.la

bin_PROGRAMS = iog

headerdir = $(prefix)/include/iog-@IOG_API_VERSION@/iog
header_DATA = $(INST_H_FILES)

MARSHAL_OUTPUT = 	\
	iog-marshal.h	\
	iog-marshal.c

NOINST_H_FILES =			\
	iog-session.h			\
	iog-util.h			\
	iog-pixbuf-util.h		\
	iog-preferences-dialog.h	\
	iog-config-keys.h		\
	iog-image-jpeg.h		\
	iog-image-private.h		\
	iog-uri-converter.h		\
	iog-metadata-reader.h		\
	iog-metadata-reader-jpg.h	\
	iog-metadata-reader-png.h	\
	iog-save-as-dialog-helper.h	\
	iog-print-image-setup.h         \
	iog-print-preview.h             \
	iog-print.h			\
	iog-module.h             	\
	iog-plugin-manager.h            \
	iog-plugin-engine.h             \
	iog-close-confirmation-dialog.h	\
	zoom.h

if ENABLE_PYTHON
NOINST_H_FILES += 			\
	iog-python-module.h		\
	iog-python-plugin.h
endif

INST_H_FILES =				\
	iog-application.h		\
	iog-debug.h			\
	iog-window.h			\
	iog-sidebar.h			\
	iog-dialog.h			\
	iog-properties-dialog.h		\
	iog-error-message-area.h	\
	iog-file-chooser.h		\
	iog-statusbar.h			\
	iog-thumb-nav.h			\
	iog-transform.h			\
	iog-image.h			\
	iog-enums.h			\
	iog-image-save-info.h		\
	iog-scroll-view.h		\
	iog-thumb-view.h		\
	iog-list-store.h		\
	iog-thumbnail.h			\
	iog-job-queue.h			\
	iog-clipboard-handler.h		\
	iog-jobs.h			\
	iog-plugin.h

libiog_c_files =			\
	iog-application.c		\
	iog-session.c			\
	iog-debug.c			\
	iog-util.c			\
	iog-pixbuf-util.c		\
	iog-window.c			\
	iog-sidebar.c			\
	iog-dialog.c			\
	iog-preferences-dialog.c	\
	iog-properties-dialog.c		\
	iog-error-message-area.c	\
	iog-file-chooser.c		\
	iog-statusbar.c			\
	iog-thumb-nav.c			\
	iog-transform.c			\
	iog-image.c			\
	iog-image-jpeg.c		\
	iog-image-save-info.c		\
	iog-scroll-view.c		\
	iog-thumb-view.c		\
	iog-list-store.c		\
	iog-thumbnail.c			\
	iog-job-queue.c			\
	iog-jobs.c			\
	iog-uri-converter.c		\
	iog-metadata-reader.c		\
	iog-metadata-reader-jpg.c	\
	iog-metadata-reader-png.c	\
	iog-save-as-dialog-helper.c	\
	iog-print-image-setup.c         \
	iog-print-preview.c           	\
	iog-print.c			\
	iog-module.c            	\
	iog-close-confirmation-dialog.c \
	iog-plugin.c            	\
	iog-plugin-manager.c            \
	iog-plugin-engine.c            	\
	iog-clipboard-handler.c		\
	zoom.c

libiog_la_SOURCES =			\
	$(libiog_c_files)		\
	$(BUILT_SOURCES)		\
	$(NOINST_H_FILES)		\
	$(INST_H_FILES)

if HAVE_EXIF
INST_H_FILES +=				\
	iog-exif-util.h			\
	iog-exif-details.h
libiog_c_files +=			\
	iog-exif-util.c			\
	iog-exif-details.c
endif

if ENABLE_PYTHON
libiog_la_SOURCES += 			\
	iog-python-module.c		\
	iog-python-module.h		\
	iog-python-plugin.c		\
	iog-python-plugin.h
endif

if HAVE_EXEMPI
# We need to make sure iog-exif-details.h
# is only listed once in INST_H_FILES
# or the build will break with automake-1.11
if !HAVE_EXIF
INST_H_FILES +=				\
	iog-exif-details.h
endif !HAVE_EXIF
libiog_la_SOURCES +=			\
	iog-exif-details.c
endif HAVE_EXEMPI

libiog_la_CPPFLAGS =						\
	-I$(top_srcdir)/jpegutils				\
	-I$(top_srcdir)/cut-n-paste/toolbar-editor		\
	-I$(top_srcdir)/cut-n-paste/totem-screensaver		\
	-DG_LOG_DOMAIN=\"IOG\"					\
	-DIOG_DATA_DIR=\""$(pkgdatadir)"\"			\
	-DIOG_LOCALE_DIR=\""$(datadir)/locale"\"		\
	-DIOG_PLUGIN_DIR=\""$(libdir)/iog/plugins"\"

libiog_la_CFLAGS =						\
	$(IOG_CFLAGS)						\
	$(WARN_CFLAGS)

libiog_la_LIBADD = \
	$(IOG_LIBS)

if HAVE_LCMS
libiog_la_CFLAGS += 	\
	$(X11_CFLAGS)

libiog_la_LIBADD += \
	$(X11_LIBS)
endif

if ENABLE_PYTHON
libiog_la_CFLAGS += 			\
	$(NO_STRICT_ALIASING_CFLAGS)	\
	$(PYGTK_CFLAGS)			\
	$(PYTHON_CFLAGS)		\
	$(AM_CFLAGS)

libiog_la_LIBADD += \
	$(top_builddir)/bindings/python/iog.la
endif

iog_SOURCES = main.c

iog_CFLAGS = 							\
	-I$(top_srcdir)/cut-n-paste/toolbar-editor		\
	-I$(top_srcdir)/cut-n-paste/totem-screensaver		\
	$(IOG_CFLAGS)						\
	-DIOG_DATA_DIR=\""$(pkgdatadir)"\"			\
	-DIOG_LOCALE_DIR=\""$(datadir)/locale"\"		\
	$(INTROSPECTION_CFLAGS)

iog_LDADD = 				\
	libiog.la			\
	$(IOG_LIBS)			\
	$(LIBJPEG)			\
	$(toolbar_LIB)			\
	$(screensaver_LIB)		\
	$(jpeg_LIB)			\
	$(INTROSPECTION_LIBS)

BUILT_SOURCES = 			\
	iog-enum-types.c		\
	iog-enum-types.h		\
	$(MARSHAL_OUTPUT)

iog-enum-types.h: iog-enum-types.h.template $(INST_H_FILES) $(GLIB_MKENUMS)
	$(AM_V_GEN)(cd $(srcdir) && $(GLIB_MKENUMS) --template iog-enum-types.h.template $(INST_H_FILES)) > $@

iog-enum-types.c: iog-enum-types.c.template $(INST_H_FILES) $(GLIB_MKENUMS)
	$(AM_V_GEN)(cd $(srcdir) && $(GLIB_MKENUMS) --template iog-enum-types.c.template $(INST_H_FILES)) > $@

iog-marshal.h: iog-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN)$(GLIB_GENMARSHAL) $< --header --internal --prefix=iog_marshal > $@

iog-marshal.c: iog-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN)$(GLIB_GENMARSHAL) $< --body --header --prefix=iog_marshal > $@

EXTRA_DIST = 				\
	iog-enum-types.h.template	\
	iog-enum-types.c.template	\
	iog-marshal.list

if HAVE_DBUS

BUILT_SOURCES += iog-application-service.h

EXTRA_DIST += iog-application-service.xml

iog-application-service.h: iog-application-service.xml
	$(AM_V_GEN)dbus-binding-tool --prefix=iog_application --mode=glib-server --output=iog-application-service.h $<

endif

CLEANFILES = $(BUILT_SOURCES)

if HAVE_INTROSPECTION
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS = Iog-1.0.gir
INTROSPECTION_SCANNER_ARGS = -I$(top_srcdir) --warn-all $(libiog_la_CPPFLAGS)

Iog-1.0.gir: iog
Iog_1_0_gir_INCLUDES = Gtk-${GTK_API_VERSION}
Iog_1_0_gir_FILES = $(INST_H_FILES) $(libiog_c_files) $(BUILT_SOURCES)
Iog_1_0_gir_VERSION = 1.0
Iog_1_0_gir_PROGRAM = $(builddir)/iog
Iog_1_0_gir_NAMESPACE = Iog
Iog_1_0_gir_CFLAGS = $(IOG_CFLAGS)
girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)

-include $(top_srcdir)/git.mk
