ZBAR BAR CODE READER
====================

ZBar Bar Code Reader is an open source software suite for reading bar codes
from various sources, such as video streams, image files and raw intensity
sensors. It supports EAN-13/UPC-A, UPC-E, EAN-8, Code 128, Code 93, Code 39,
Codabar, Interleaved 2 of 5 and QR Code.  Included with the library are basic
applications for decoding captured bar code images and using a video device
(eg, webcam) as a bar code scanner.  For application developers, language
bindings are included for C, C++, Python and Perl as well as GUI widgets for
Qt, GTK and PyGTK.

Check the ZBar home page for the latest release, mailing lists, etc.

  * <https://github.com/mchehab/zbar>

License information can be found in 'COPYING'.

Once built, the Windows binaries will use binaries of several supporting
libraries, each one with its own copyright, license and source code locations.

It follows a non-exhaustive list of those components:

  * The GNU libiconv character set conversion library

    Copyright (C) since 1999 Free Software Foundation, Inc.

    Licensed under LGPL.  The source code is available from

      * <http://www.gnu.org/software/libiconv>

  * The ImageMagick software imaging library

    Copyright since 1999 ImageMagick Studio LLC

    Licensed under a derived Apache 2.0 license:

      * https://imagemagick.org/script/license.php

    The source code is available from

      * <http://imagemagick.org>

  * The libxml2 XML C parser and toolkit

    Copyright (C) since 1998 Daniel Veillard.

    Licensed under the MIT license.

    The source code is available from:

      * <http://xmlsoft.org>

  * JPEG library

    The Independent JPEG Group's software's version is:

       Copyright (C) since 1991 Thomas G. Lane, Guido Vollbeding.

    Libjpeg-turbo has additional copyrights:

       Copyright (C) since 2009 D. R. Commander.
       Copyright (C) since 2015 Google, Inc.

    Licensed under BSD-style licenses with their own terms:

      * https://www.ijg.org/files/README
      * https://github.com/libjpeg-turbo/libjpeg-turbo/blob/master/LICENSE.md

    The source code is available from:

      * <http://www.ijg.org>

  * libtiff, a library for reading and writing TIFF

    Copyright (c) since 1988 Sam Leffler

    Copyright (c) since 1991 Silicon Graphics, Inc.

    Licensed under a BSD-style license.

    The source code is available from

      * <http://www.libtiff.org>

  * libpng, the official PNG reference library

    Copyright (c) since 1998 Glenn Randers-Pehrson

    Licensed under a BSD-style license.

    The source code is available from

      * <http://www.libpng.org/pub/png/libpng.html>

  * The zlib general purpose compression library

    Copyright (C) since 1995 Jean-loup Gailly and Mark Adler.

    Licensed under a BSD-style license.

    The source code is available from

      * <http://zlib.net>

  * The bzip2 compression library

    Copyright (C) since 1996 Julian Seward.

    Licensed under a BSD-style license.

    The source code is available from

      * <http://bzip.org>

  * Depending on how this is packaged, other licenses may apply


RUNNING
=======

This version of the package includes *only command line programs*.
(The graphical interface is scheduled for a future release)

Invoke Start -> Programs -> ZBar Bar Code Reader -> Start ZBar Command Prompt
to open a shell that has the zbarimg and zbarcam commands available
(in the PATH).

To start the webcam reader using the default camera, type:

    zbarcam

To decode an image file, type eg:

    zbarimg -d examples\barcode.png

For basic command instructions, type:

    zbarimg --help
    zbarcam --help

Check the manual for more details.


REPORTING BUGS
==============

Bugs can be reported at the GitHub project page

  * <https://github.com/mchehab/zbar>

Please include the ZBar version number and a detailed description of
the problem.  You'll probably have better luck if you're also familiar
with the concepts from:

  * <http://www.catb.org/~esr/faqs/smart-questions.html>
