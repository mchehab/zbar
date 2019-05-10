lib_LTLIBRARIES = libzbargtk.la
libzbargtk_la_CPPFLAGS = -I$(srcdir) -I$(top_srcdir)/include $(GTK_CFLAGS) $(AM_CPPFLAGS)
libzbargtk_la_LDFLAGS = -version-info $(ZGTK_LIB_VERSION) \
    -export-symbols-regex "^zbar_.*" $(AM_LDFLAGS) -no-undefined
libzbargtk_la_LIBADD = $(GTK_LIBS) ../zbar/libzbar.la $(AM_LIBADD)

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

ZBar-1.0.gir: $(lib_LTLIBRARIES) ../zbarcam/zbarcam-gtk

INTROSPECTION_SCANNER_ENV = GI_SCANNER_DISABLE_CACHE=yes
INTROSPECTION_SCANNER_ARGS = --warn-all \
			     --symbol-prefix=zbar \
			     --identifier-prefix=zbar_ \
			     --identifier-prefix=ZBar

INTROSPECTION_GIRS = ZBar-1.0.gir
ZBar_1_0_gir_NAMESPACE = ZBar
ZBar_1_0_gir_VERSION = 1.0
ZBar_1_0_gir_PROGRAM = $(builddir)/gtk-gir-dump
ZBar_1_0_gir_LIBS = $(lib_LTLIBRARIES)
ZBar_1_0_gir_FILES = ../include/zbar/zbargtk.h zbargtk.c
ZBar_1_0_gir_INCLUDES = Gtk-@GTK_VERSION_MAJOR@ Gdk-@GTK_VERSION_MAJOR@
ZBar_1_0_gir_CFLAGS= $(libzbargtk_la_CPPFLAGS)

if HAVE_INTROSPECTION

noinst_bindir = @abs_builddir@
noinst_bin_PROGRAMS = gtk-gir-dump
gtk_gir_dump_SOURCES = gtk-gir-dump.c
gtk_gir_dump_CPPFLAGS = $(GTK_CFLAGS) $(AM_CPPFLAGS) $(INTROSPECTION_CFLAGS)
gtk_gir_dump_LDADD = $(GTK_LIBS) libzbargtk.la ../zbar/libzbar.la \
	$(AM_LDADD) $(INTROSPECTION_LIBS)

girdir = $(INTROSPECTION_GIRDIR)
dist_gir_DATA = $(INTROSPECTION_GIRS)
typelibdir = $(INTROSPECTION_TYPELIBDIR)
typelib_DATA = ZBar-1.0.typelib

CLEANFILES += $(dist_gir_DATA) $(typelib_DATA)

endif
