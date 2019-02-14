ZBAR BAR CODE READER
====================

ZBar Bar Code Reader is an open source software suite for reading bar
codes from various sources, such as video streams, image files and raw
intensity sensors. It supports EAN-13/UPC-A, UPC-E, EAN-8, Code 128,
Code 93, Code 39, Codabar, Interleaved 2 of 5, QR Code and SQ Code.

Included with the library are basic applications for decoding captured bar
code images and using a video device (eg, webcam) as a bar code scanner.
For application developers, language bindings are included for C, C++,
Python 2 and Perl as well as GUI widgets for Qt, GTK and PyGTK 2.0.

Zbar also supports sending the scanned codes via dbus, allowing its
integration with other applications.

Check the ZBar home page for the latest release, mailing lists, etc.:

- <https://github.com/mchehab/zbar>

License information can be found in `COPYING`.


BUILDING
========

See `INSTALL.md` for generic configuration and build instructions.

The scanner/decoder library itself only requires a few standard
library functions which should be available almost anywhere.

The zbarcam program uses the video4linux API (v4l1 or v4l2) to access
the video device.  This interface is part of the linux kernel, a 3.16
kernel or upper is recommended for full support.  More information is
available at:

- <http://www.linuxtv.org/wiki/>

`pkg-config` is used to locate installed libraries.  You should have
installed `pkg-config` if you need any of the remaining components.
pkg-config may be obtained from:

- <http://pkg-config.freedesktop.org/>

The `zbarimg` program uses `ImageMagick` to read image files in many
different formats.  You will need at least `ImageMagick` version 6.2.6
if you want to scan image files.  `ImageMagick` may be obtained from:

- <http://www.imagemagick.org/>

The Qt widget requires Qt4 or Qt5. You will need Qt if you would like to
use or develop a Qt GUI application with an integrated bar code
scanning widget. Qt4 may be obtained from:

- <https://www.qt.io/>

The GTK+ widget requires GTK+-2.x.  You will need GTK+ if you would
like to use or develop a GTK+ GUI application with an integrated bar
code scanning widget.  GTK+ may be obtained from:

- <http://www.gtk.org/>

The PyGTK 2.0 wrapper for the GTK+ widget requires Python 2, PyGTK.
You will need both if you would like to use or develop a PyGTK GUI
application with an integrated bar code scanning widget.  PyGTK may be
obtained from:

- <http://www.pygtk.org/>

The Python bindings require Python 2.  You will need Python and PIL
if you would like to scan images or video directly using Python.
Python is available from:

- <http://python.org/>

The Perl bindings require Perl (version?).  You will need Perl if you
would like to scan images or video directly using Perl.  Perl is
available from:

- <http://www.perl.org/>

If required libraries are not available you may disable building for
the corresponding component using configure (see configure --help).

The Perl bindings must be built separately after installing the
library.  see:

- `perl/README`


RUNNING
=======

`make install` will install the library and application programs.  Run
`zbarcam-qt` or `zbarcam` to start the video scanner. Use `zbarimg <file>`
to decode a saved image file.

Check the manual to find specific options for each program.

DBUS TESTING
============

In order to test if dbus is working, you could use:

	$ dbus-monitor --system interface=org.linuxtv.Zbar1.Code

or build the test programs with:

	$ make test_progs

And run:
	$ ./test/test_dbus

With that, running this command on a separate shell:

	$ ./zbarimg/zbarimg examples/code-128.png
	CODE-128:https://github.com/mchehab/zbar
	scanned 1 barcode symbols from 1 images in 0.01 seconds

Will produce this output at test_dbus shell window:

	Waiting for Zbar events
	Type = CODE-128
	Value = https://github.com/mchehab/zbar

REPORTING BUGS
==============

Bugs can be reported on the project page:

- <https://github.com/mchehab/zbar>

Please include the ZBar version number and a detailed description of
the problem.  You'll probably have better luck if you're also familiar
with the concepts from:

- <http://www.catb.org/~esr/faqs/smart-questions.html>
