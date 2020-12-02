AM_CPPFLAGS = $(IOG_CFLAGS) \
	$(EXIF_CFLAGS)

noinst_LTLIBRARIES = libiog-jpegutils.la

libiog_jpegutils_la_SOURCES =		\
	jpegint.h			\
	transupp.h

if HAVE_LIBJPEG_80
libiog_jpegutils_la_SOURCES +=		\
	jpegint-8a.h			\
	transupp-8a.c			\
	transupp-8a.h
else
libiog_jpegutils_la_SOURCES +=		\
	transupp-6b.c			\
	transupp-6b.h
endif

EXTRA_DIST = README.6b README.8a

-include $(top_srcdir)/git.mk
