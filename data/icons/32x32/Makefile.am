size = 32x32
actioniconsdir = $(pkgdatadir)/icons/hicolor/$(size)/actions
appiconsdir = $(datadir)/icons/hicolor/$(size)/apps

dist_appicons_DATA = \
	apps/iog.png
dist_actionicons_DATA = \
	actions/iog-plugin.png	  	\
	actions/iog-image-collection.png  \
	actions/slideshow-play.png

noinst_DATA = \
	apps/iog.svg  \
	actions/iog-image-collection.svg  \
	actions/iog-plugin.svg  \
	actions/slideshow-play.svg

EXTRA_DIST = $(noinst_DATA)

-include $(top_srcdir)/git.mk
