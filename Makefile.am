moduledir = ${libdir}/gtk-2.0/@GTK_VERSION@/immodules
## Makefile.am for gtk+/modules/input 
if PLATFORM_WIN32
no_undefined = -no-undefined
endif

SUBDIRS = po

INCLUDES = @GTK_DEP_CFLAGS@ -DIM_LOCALEDIR=\"${datadir}/locale\"

DEPS =

LDADDS = @GTK_DEP_LIBS@ -lcanna

EXTRA_DIST=	\
	im-canna.spec \
	tests/Makefile \
	tests/README \
	tests/textview.c

im_canna_la_LDFLAGS = -rpath $(moduledir) -avoid-version -module $(no_undefined)

im_canna_la_SOURCES = \
		imcanna.c \
		imsss.c \
		imsss.h \
		furigana.h \
		chartype.h \
		chartype.c \
		im-canna-intl.h

im_canna_la_LIBADD = $(LDADDS)

module_LTLIBRARIES =				\
	im-canna.la

module_list = ${libdir}/gtk-2.0/@GTK_VERSION@/immodules.cache

gtk.immodules: Makefile.am $(module_LTLIBRARIES)
	${bindir}/gtk-query-immodules-2.0 *.la > gtk.immodules

all-local: gtk.immodules

# For cut the cost on the install process
install-etc:
	${bindir}/gtk-query-immodules-2.0 > ${module_list}
