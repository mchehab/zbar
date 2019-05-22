lib_LTLIBRARIES = libzbargtk.la
libzbargtk_la_CPPFLAGS = $(GTK_CFLAGS) $(AM_CPPFLAGS)
libzbargtk_la_LDFLAGS = -version-info $(ZGTK_LIB_VERSION) \
    -export-symbols-regex "^zbar_.*" $(AM_LDFLAGS) -no-undefined
libzbargtk_la_LIBADD = $(GTK_LIBS) ../zbar/libzbar.la $(AM_LIBADD)
libzbargtk_la_DEPENDENCIES = ../zbar/libzbar.la


if HAVE_X
libzbargtk_la_CPPFLAGS += -DHAVE_X
endif

dist_libzbargtk_la_SOURCES = zbargtk.c zbargtkprivate.h
nodist_libzbargtk_la_SOURCES = zbarmarshal.c zbarmarshal.h
BUILT_SOURCES = zbarmarshal.c zbarmarshal.h
CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = zbarmarshal.list

%.h: %.list
	$(GLIB_GENMARSHAL) --g-fatal-warnings --prefix=zbar_marshal \
	    --header $^ > $@

%.c: %.list
	$(GLIB_GENMARSHAL) --g-fatal-warnings --prefix=zbar_marshal \
	    --body $^ > $@

../zbar/libzbar.la:
	$(MAKE) -C $(abs_top_srcdir) zbar/libzbar.la

../zbarcam/zbarcam-gtk: libzbargtk.la
	$(MAKE) -C $(abs_top_srcdir) zbarcam/zbarcam-gtk

# GObject Introspection

include $(INTROSPECTION_MAKEFILE)



# NOTE:
#
# At least with Fedora 30 builds using mock (e. g. inside a chroot and
# having the build dir different than the source dir, using
# gobject-introspection-1.60, there is a bug with GIR file generation:
# sometimes, g-ir-scanner is not capable of producing a C file that would
# be loading libzbargtk.so.0. So, it fails with:
#
#	error while loading shared libraries: libzbargtk.so.0:
#		cannot open shared object file: No such file or directory
#
# I suspect that it has something to do with libtool-2.4.6.

# The fix is hackish, but it should be safe: it should manually include
# the paths where libtool generate those at INTROSPECTION_SCANNER_ARGS.
#
# It should be noticed that, this shouldn't affect a non-buggy environment,
# so, better to be safe than sorry.
INTROSPECTION_SCANNER_ARGS = --warn-all \
			     --symbol-prefix=zbar \
			     --identifier-prefix=zbar_ \
			     --identifier-prefix=ZBar \
			     --library-path=$(abs_builddir)/.libs \
			     --library-path=$(abs_top_builddir)/zbar/.libs

# Just in case
INTROSPECTION_SCANNER_ENV = GI_SCANNER_DISABLE_CACHE=yes

INTROSPECTION_GIRS = ZBar-1.0.gir

ZBar_1_0_gir_NAMESPACE = ZBar
ZBar_1_0_gir_VERSION = 1.0
ZBar_1_0_gir_LIBS = $(lib_LTLIBRARIES) $(top_builddir)/zbar/libzbar.la
ZBar_1_0_gir_FILES = $(top_builddir)/include/zbar/zbargtk.h zbargtk.c
ZBar_1_0_gir_INCLUDES = Gtk-@GTK_VERSION_MAJOR@ Gdk-@GTK_VERSION_MAJOR@
ZBar_1_0_gir_CFLAGS = $(libzbargtk_la_CPPFLAGS)

# This may generate some warnings, but it is needed for "make dist"
ZBar-1.0.gir: $(lib_LTLIBRARIES)

if HAVE_INTROSPECTION

girdir = $(INTROSPECTION_GIRDIR)
dist_gir_DATA = $(INTROSPECTION_GIRS)
typelibdir = $(INTROSPECTION_TYPELIBDIR)
typelib_DATA = ZBar-1.0.typelib

CLEANFILES += $(dist_gir_DATA) $(typelib_DATA)

endif
