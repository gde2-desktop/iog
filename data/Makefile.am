SUBDIRS = pixmaps icons

@INTLTOOL_DESKTOP_RULE@

DESKTOP_IN_FILES= iog.desktop.in.in
DESKTOP_FILES= $(DESKTOP_IN_FILES:.desktop.in.in=.desktop)

desktopdir = $(datadir)/applications
desktop_DATA = $(DESKTOP_FILES)

@INTLTOOL_XML_RULE@
appdatadir = $(datadir)/appdata
appdata_in_files = iog.appdata.xml.in
appdata_DATA = $(appdata_in_files:.xml.in=.xml)

gsettings_ENUM_NAMESPACE = org.gde2.iog
gsettings_ENUM_FILES = $(top_srcdir)/src/iog-scroll-view.h	\
                       $(top_srcdir)/src/iog-window.h

gsettings_SCHEMAS = org.gde2.iog.gschema.xml
@GSETTINGS_RULES@

uidir = $(pkgdatadir)
ui_DATA = \
	iog-image-properties-dialog.ui \
	iog-multiple-save-as-dialog.ui \
	iog-preferences-dialog.ui \
	iog-ui.xml \
	iog-toolbar.xml

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = iog.pc

themedir = $(pkgdatadir)
theme_DATA = \
        gtkrc \
        iog.css

EXTRA_DIST = \
        $(theme_DATA) \
        $(ui_DATA) \
        $(DESKTOP_IN_FILES) \
        $(appdata_in_files)

CLEANFILES = \
        $(appdata_DATA)

DISTCLEANFILES = \
        $(DESKTOP_FILES) \
        $(gsettings_SCHEMAS)

-include $(top_srcdir)/git.mk
