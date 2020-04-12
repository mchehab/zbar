bin_PROGRAMS += zbarcam/zbarcam
zbarcam_zbarcam_SOURCES = zbarcam/zbarcam.c
zbarcam_zbarcam_LDADD = zbar/libzbar.la
zbarcam_zbarcam_CPPFLAGS = $(AM_CPPFLAGS)
# automake bug in "monolithic mode"?
CLEANFILES += zbarcam/.libs/zbarcam zbarcam/moc_zbarcam_qt.h

if HAVE_GTK
if !WIN32
bin_PROGRAMS += zbarcam/zbarcam-gtk
zbarcam_zbarcam_gtk_SOURCES = zbarcam/zbarcam-gtk.c zbarcam/scan_video.c
zbarcam_zbarcam_gtk_CPPFLAGS = $(GTK_CFLAGS) $(AM_CPPFLAGS)
zbarcam_zbarcam_gtk_LDADD = $(GTK_LIBS) gtk/libzbargtk.la zbar/libzbar.la \
	$(AM_LDADD)

endif
endif

if HAVE_QT
bin_PROGRAMS += zbarcam/zbarcam-qt

zbarcam_zbarcam_qt_SOURCES = zbarcam/zbarcam-qt.cpp zbarcam/scan_video.c
nodist_zbarcam_zbarcam_qt_SOURCES = zbarcam/moc_zbarcam_qt.h
zbarcam_zbarcam_qt_CPPFLAGS = -Izbarcam $(QT_CFLAGS) $(AM_CPPFLAGS)
zbarcam_zbarcam_qt_LDADD = $(QT_LIBS) qt/libzbarqt.la $(AM_LDADD)
BUILT_SOURCES += $(nodist_zbarcam_zbarcam_qt_SOURCES)
DISTCLEANFILES += $(nodist_zbarcam_zbarcam_qt_SOURCES) zbarcam/moc_zbarcam_qt.h


zbarcam/moc_zbarcam_qt.h: zbarcam/zbarcam-qt.cpp
	$(MOC) -i $(zbarcam_zbarcam_qt_CPPFLAGS) $< -o $@
endif


if WIN32
zbarcam_zbarcam_SOURCES += zbarcam/zbarcam.rc
zbarcam_zbarcam_LDADD += zbarcam/zbarcam-rc.o @LTLIBINTL@

if WITH_DIRECTSHOW
zbarcam_zbarcam_CPPFLAGS += -DDIRECTSHOW
endif

endif
