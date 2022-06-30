pyexec_LTLIBRARIES += pygtk/zbargtk.la
pygtk_zbargtk_la_CPPFLAGS = \
    $(GTK_CFLAGS) $(PYTHON_CFLAGS) $(PYGOBJECT_CFLAGS) $(AM_CPPFLAGS)
pygtk_zbargtk_la_LDFLAGS = \
    -avoid-version -module $(PYTHON_LDFLAGS) $(PYGOBJECT_LDLAGS) $(AM_LDFLAGS)
pygtk_zbargtk_la_LIBADD = gtk/libzbargtk.la $(AM_LIBADD)
pygtk_zbargtk_la_DEPENDENCIES = gtk/libzbargtk.la
pygtk_zbargtk_la_SOURCES = pygtk/zbargtkmodule.c
