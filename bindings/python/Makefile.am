noinst_LTLIBRARIES = \
	iog.la

nodist_iog_la_SOURCES = \
	iog.c

iog_la_LDFLAGS = \
	-module -avoid-version

iog_la_LIBADD = 		\
	$(PYTHON_LIB_LOC)       \
	$(PYTHON_LIBS)		\
	$(PYTHON_EXTRA_LIBS)	\
	$(PYGTK_LIBS)

iog_la_CFLAGS = 					\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	-I$(top_srcdir)/src				\
	-I$(top_builddir)/src				\
	-I$(top_srcdir)/cut-n-paste/toolbar-editor	\
	$(IOG_CFLAGS)					\
	$(NO_STRICT_ALIASING_CFLAGS)			\
	$(PYGTK_CFLAGS)					\
	$(PYTHON_CFLAGS)				\
	$(AM_CFLAGS)

$(top_builddir)/src/iog-enum-types.h:
	$(AM_V_GEN)cd $(top_builddir)/src && $(MAKE) iog-enum-types.h

iog.c: iog.defs iog.override $(top_builddir)/src/iog-enum-types.h
	$(AM_V_GEN)( cd $(srcdir) && $(PYGOBJECT_CODEGEN) \
		--register $(PYGOBJECT_DEFSDIR)/gio-types.defs \
		--register $(PYGTK_DEFSDIR)/pango-types.defs \
		--register $(PYGTK_DEFSDIR)/gdk-types.defs \
		--register $(PYGTK_DEFSDIR)/gtk-types.defs \
		--override $*.override \
		--prefix py$* $(<F) ) > $@

BINDING_IOG_HEADERS_SRCDIR_IN = 			\
	src/iog-application.h				\
	src/iog-window.h				\
	src/iog-dialog.h				\
	src/iog-properties-dialog.h			\
	src/iog-message-area.h				\
	src/iog-statusbar.h				\
	src/iog-thumb-nav.h				\
	src/iog-image.h					\
	src/iog-scroll-view.h				\
	src/iog-thumb-view.h				\
	src/iog-list-store.h				\
	src/iog-job-queue.h				\
	src/iog-jobs.h					\
	src/iog-plugin.h

BINDING_HEADERS_BUILDDIR_IN =

BINDING_IOG_HEADERS_SRCDIR := $(addprefix $(top_srcdir)/,$(BINDING_IOG_HEADERS_SRCDIR_IN))

BINDING_HEADERS_BUILDDIR   := $(addprefix $(top_builddir)/,$(BINDING_HEADERS_BUILDDIR_IN))

regenerate-python-binding:
	$(PYGTK_H2DEF) $(sort $(BINDING_IOG_HEADERS_SRCDIR) $(BINDING_HEADERS_BUILDDIR)) > iog.defs.new

BUILT_SOURCES = \
	iog.c

EXTRA_DIST = 		\
	iog.override	\
	iog.defs

CLEANFILES = $(BUILT_SOURCES)

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)

-include $(top_srcdir)/git.mk
