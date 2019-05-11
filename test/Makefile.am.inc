check_PROGRAMS += test/test_decode
test_test_decode_SOURCES = test/test_decode.c test/pdf417_encode.h
test_test_decode_CFLAGS = -Wno-unused $(AM_CFLAGS)
test_test_decode_LDADD = zbar/libzbar.la $(AM_LDADD)

TEST_IMAGE_SOURCES = test/test_images.c test/test_images.h

check_PROGRAMS += test/test_convert
test_test_convert_SOURCES = test/test_convert.c $(TEST_IMAGE_SOURCES)
test_test_convert_LDADD = zbar/libzbar.la $(AM_LDADD)

#check_PROGRAMS += test/test_window
#test_test_window_SOURCES = test/test_window.c $(TEST_IMAGE_SOURCES)
#test_test_window_CPPFLAGS = -I$(srcdir)/zbar $(AM_CPPFLAGS)
#test_test_window_LDADD = zbar/libzbar.la $(AM_LDADD)

if HAVE_VIDEO
check_PROGRAMS += test/test_video
test_test_video_SOURCES = test/test_video.c $(TEST_IMAGE_SOURCES)
test_test_video_LDADD = zbar/libzbar.la $(AM_LDADD)
endif

check_PROGRAMS += test/test_proc
test_test_proc_SOURCES = test/test_proc.c $(TEST_IMAGE_SOURCES)
test_test_proc_LDADD = zbar/libzbar.la $(AM_LDADD)

check_PROGRAMS += test/test_cpp
test_test_cpp_SOURCES = test/test_cpp.cpp
test_test_cpp_LDADD = zbar/libzbar.la $(AM_LDADD)

check_PROGRAMS += test/test_cpp_img
test_test_cpp_img_SOURCES = test/test_cpp_img.cpp $(TEST_IMAGE_SOURCES)
test_test_cpp_img_LDADD = zbar/libzbar.la $(AM_LDADD)

if HAVE_JPEG
check_PROGRAMS += test/test_jpeg
test_test_jpeg_SOURCES = test/test_jpeg.c
test_test_jpeg_LDADD = zbar/libzbar.la $(AM_LDADD)
endif

if HAVE_MAGICK
EXTRA_PROGRAMS += test/dbg_scan
test_dbg_scan_SOURCES = test/dbg_scan.cpp
test_dbg_scan_CPPFLAGS = $(MAGICK_CFLAGS) $(AM_CPPFLAGS)
test_dbg_scan_LDADD = $(MAGICK_LIBS) -lMagick++ zbar/libzbar.la $(AM_LDADD)
endif

if HAVE_DBUS
check_PROGRAMS += test/test_dbus
test_test_dbus_SOURCES = test/test_dbus.c
test_test_dbus_LDFLAGS = $(DBUS_LIBS)
endif

EXTRA_DIST += test/test_pygtk.py test/test_perl.pl test/test_gi.py test/test_python.py

# automake bug in "monolithic mode"?
CLEANFILES += test/.libs/test_decode test/.libs/test_proc \
    test/.libs/test_convert test/.libs/test_window \
    test/.libs/test_video test/.libs/dbg_scan test/.libs/test_gtk


# Images that work out of the box without needing to enable
# an specific symbology
NORMAL_IMAGES = codabar.png code-128.png code-39.png code-93.png \
		databar.png databar-exp.png ean-13.png ean-8.png i2-5.png \
		qr-code.png sqcode1-generated.png sqcode1-scanned.png

EXAMPLES = @abs_top_builddir@/examples
ZBARIMG = @abs_top_builddir@/zbarimg/zbarimg --nodbus

gen_checksum: all
	for i in $(NORMAL_IMAGES); do $(ZBARIMG) $(EXAMPLES)/$$i 2>/dev/null|sha1sum|sed "s,-,zbarimg $$i,"; done >$(EXAMPLES)/sha1sum
	$(ZBARIMG) -Sean2.enable $(EXAMPLES)/ean-2.png 2>/dev/null|sha1sum|sed "s,-,zbarimg -Sean2.enable ean-2.png," >>$(EXAMPLES)/sha1sum
	$(ZBARIMG) -Sean5.enable $(EXAMPLES)/ean-5.png 2>/dev/null|sha1sum|sed "s,-,zbarimg -Sean5.enable ean-5.png," >>$(EXAMPLES)/sha1sum
	$(ZBARIMG) -Sisbn10.enable $(EXAMPLES)/ean-13.png 2>/dev/null|sha1sum|sed "s,-,zbarimg -Sisbn10.enable ean-13.png," >>$(EXAMPLES)/sha1sum
	$(ZBARIMG) -Sisbn13.enable $(EXAMPLES)/ean-13.png 2>/dev/null|sha1sum|sed "s,-,zbarimg -Sisbn13.enable ean-13.png,"  >>$(EXAMPLES)/sha1sum
	$(ZBARIMG) -Supca.enable $(EXAMPLES)/code-upc-a.png 2>/dev/null|sha1sum|sed "s,-,zbarimg -Supca.enable code-upc-a.png,"  >>$(EXAMPLES)/sha1sum
	$(ZBARIMG) -Stest-inverted $(EXAMPLES)/qr-code-inverted.png 2>/dev/null|sha1sum|sed "s,-,zbarimg -Stest-inverted qr-code-inverted.png,"  >>$(EXAMPLES)/sha1sum

test_progs: $(check_PROGRAMS)
	@$(MAKE) $(check_PROGRAMS)

# Require X11 to work
check-cpp: test/test_cpp_img
	@abs_top_builddir@/test/test_cpp_img

check-decoder: test/test_decode
	@abs_top_builddir@/test/test_decode -q

regress-decoder: test/test_decode
	@abs_top_builddir@/test/test_decode -q -n 100000

check-images-py: zbarimg/zbarimg
	@PYTHON@ @abs_top_srcdir@/test/barcodetest.py

check-images: zbarimg/zbarimg
	@abs_top_builddir@/test/test_examples.sh

check-convert: test/test_convert
	@abs_top_srcdir@/test/test_convert
	@if [ "`sha1sum /tmp/base.I420.zimg |cut -d' ' -f 1`" != \
	     "d697b0bb84617bef0f6413b3e5537ee38ba92312" ]; then \
	   echo "convert FAILED"; else echo "convert PASSED."; fi
	@rm /tmp/base.I420.zimg 2>/dev/null

if HAVE_PYGTK2
check-pygtk: pygtk/zbarpygtk.la
	PYTHONPATH=@abs_top_srcdir@/pygtk/.libs/ \
		@PYTHON@ @abs_top_srcdir@/test/test_pygtk.py
else
check-pygtk:
endif

if HAVE_PYTHON
check-python: python/zbar.la
	PYTHONPATH=@abs_top_srcdir@/python/.libs/ \
		@PYTHON@ @abs_top_srcdir@/test/test_python.py \
		'@abs_top_srcdir@/examples/ean-13.png' '9789876543217'
else
check-python:
endif

check-gi: gtk/ZBar-1.0.typelib
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):@abs_top_srcdir@/gtk/.libs:@abs_top_srcdir@/zbar/.libs \
		GI_TYPELIB_PATH=@abs_top_srcdir@/gtk/ \
		@PYTHON@ @abs_top_srcdir@/test/test_gi.py

# Require a camera device for it to work
check-video: test/test_video
	@abs_top_srcdir@/test/test_video -q

check-jpeg: test/test_jpeg
	@abs_top_srcdir@/test/test_jpeg -q

if HAVE_DBUS
# Require a working D-Bus - may fail with containers
check-dbus: test/test_dbus
	@abs_top_builddir@/test/check_dbus.sh
else
check-dbus:
endif

if HAVE_JAVA_UNIT
check-java: zbar/libzbar.la
	JAVA_HOME=${JAVA_HOME} $(MAKE) -C java check-java
else
check-java:
endif

regress: regress-decoder

check-local: check-images-py check-decoder check-images check-java \
	     check-python regress

other-tests: check-cpp check-convert check-video check-jpeg

tests: check-local check-dbus other-tests

.NOTPARALLEL: check-local regress tests

PHONY += gen_checksum check-cpp check-decoder check-images check-dbus regress-decoder regress-images regress
