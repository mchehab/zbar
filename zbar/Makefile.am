lib_LTLIBRARIES = libzbar.la
libzbar_la_CPPFLAGS = $(AM_CPPFLAGS)
libzbar_la_LDFLAGS = -no-undefined -version-info $(LIB_VERSION) \
    -export-symbols-regex "^(zbar|_zbar.*_error)_.*" $(AM_LDFLAGS)
libzbar_la_LIBADD = $(LTLIBICONV)

libzbar_la_SOURCES = debug.h config.c \
    error.h error.c symbol.h symbol.c \
    image.h image.c convert.c \
    processor.c processor.h processor/lock.c \
    refcnt.h refcnt.c timer.h mutex.h \
    event.h thread.h \
    window.h window.c video.h video.c \
    img_scanner.h img_scanner.c scanner.c \
    decoder.h decoder.c misc.h misc.c

EXTRA_libzbar_la_SOURCES = svg.h svg.c

if ENABLE_EAN
libzbar_la_SOURCES += decoder/ean.h decoder/ean.c
endif
if ENABLE_DATABAR
libzbar_la_SOURCES += decoder/databar.h decoder/databar.c
endif
if ENABLE_CODE128
libzbar_la_SOURCES += decoder/code128.h decoder/code128.c
endif
if ENABLE_CODE93
libzbar_la_SOURCES += decoder/code93.h decoder/code93.c
endif
if ENABLE_CODE39
libzbar_la_SOURCES += decoder/code39.h decoder/code39.c
endif
if ENABLE_CODABAR
libzbar_la_SOURCES += decoder/codabar.h decoder/codabar.c
endif
if ENABLE_I25
libzbar_la_SOURCES += decoder/i25.h decoder/i25.c
endif
if ENABLE_PDF417
libzbar_la_SOURCES += decoder/pdf417.h decoder/pdf417.c \
    decoder/pdf417_hash.h
endif
if ENABLE_QRCODE
libzbar_la_SOURCES += qrcode.h \
    decoder/qr_finder.h decoder/qr_finder.c \
    qrcode/qrdec.h qrcode/qrdec.c qrcode/qrdectxt.c \
    qrcode/rs.h qrcode/rs.c \
    qrcode/isaac.h qrcode/isaac.c \
    qrcode/bch15_5.h qrcode/bch15_5.c \
    qrcode/binarize.h qrcode/binarize.c \
    qrcode/util.h qrcode/util.c
endif
if ENABLE_SQCODE
libzbar_la_SOURCES += sqcode.h sqcode.c \
    decoder/sq_finder.h decoder/sq_finder.c
endif

if WIN32

%-rc.o: %.rc
	$(RC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	$(AM_CPPFLAGS) $(CPPFLAGS) -o $@ $<
%-rc.lo: %.rc
	$(LIBTOOL) --tag=RC $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) \
	--mode=compile $(RC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	$(AM_CPPFLAGS) $(CPPFLAGS) -o $@ $<

libzbar_la_SOURCES += processor/win.c libzbar.rc
libzbar_la_CPPFLAGS += -mthreads
libzbar_la_LDFLAGS += -mthreads
# FIXME broken
libzbar_la_LIBADD += libzbar-rc.lo
else
libzbar_la_SOURCES += processor/posix.h processor/posix.c
endif

if HAVE_V4L2
libzbar_la_SOURCES += video/v4l.c video/v4l2.c
endif
if HAVE_V4L1
if !HAVE_V4L2
libzbar_la_SOURCES += video/v4l.c
endif
libzbar_la_SOURCES += video/v4l1.c
endif
if WIN32
if HAVE_VIDEO
if !WITH_DIRECTSHOW
libzbar_la_SOURCES += video/vfw.c
libzbar_la_LIBADD += -lvfw32
else
libzbar_la_SOURCES += video/dshow.c
libzbar_la_CPPFLAGS += -DCOBJMACROS
libzbar_la_LIBADD += -loleaut32 -lole32
endif
endif
endif
if !HAVE_VIDEO
libzbar_la_SOURCES += video/null.c
endif

if HAVE_LIBV4L
libzbar_la_LIBADD += $(V4L2_LIBS)
endif

if HAVE_JPEG
libzbar_la_SOURCES += jpeg.c
endif

if HAVE_X
libzbar_la_SOURCES +=  processor/x.c \
    window/x.h window/x.c window/ximage.c
libzbar_la_CPPFLAGS += $(X_CFLAGS)
libzbar_la_LDFLAGS += $(X_LIBS)
libzbar_la_LIBADD += $(X_PRE_LIBS) -lX11 $(X_EXTRA_LIBS)
if HAVE_XV
libzbar_la_SOURCES += window/xv.c
libzbar_la_LIBADD += $(XV_LIBS)
endif
else
if WIN32
libzbar_la_SOURCES += window/win.h window/win.c \
    window/dib.c
# window/vfw.c -lvfw32
libzbar_la_LIBADD += -lgdi32 -lwinmm
else
libzbar_la_SOURCES += processor/null.c window/null.c
endif
endif

if HAVE_DBUS
libzbar_la_LDFLAGS += $(DBUS_LIBS)
endif

libzbar_la_LDFLAGS += $(AM_LDFLAGS)
libzbar_la_LIBADD += $(AM_LIBADD)
