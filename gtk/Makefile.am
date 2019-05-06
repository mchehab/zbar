lib_LTLIBRARIES = libzbargtk.la
libzbargtk_la_CPPFLAGS = -I$(srcdir) $(GTK_CFLAGS) $(AM_CPPFLAGS)
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
