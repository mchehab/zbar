ZBar Barcode Reader News
========================

Version 0.23
============

  Update ZBar for it to work with updated library versions, making it
  compatible with either Gtk2 or Gtk3 and either Python2 or Python3.
  As part of the new port, it is now possible to use ZBar Gtk bindings
  not only with python2/python3, but also on other languages, as it now
  uses GObject Introspection- GIR, with is a method to allow using libraries
  developed on one language on others. Several languages support it.
  On Windows side, support for DirectShow was added.

Version 0.22.2 (2019-04-29)
===========================

  Some fixes to another set of Windows issues and to support Java 11.

Version 0.22.1 (2019-04-25)
===========================

  Some fixes to allow building ZBar on Windows with MinGw.

Version 0.22 (2019-02-20)
=========================

  Lots of improvements at zbarcam-qt, allowing it to fully configure the
  decoders that will be used, and the options that will be used at the
  decoders. Some improvements at the image scanner logic and plugin
  selection.

Version 0.21 (2019-02-12)
=========================

  Added support for SQ code, and the ability of compiling ZBar with the
  LLVM/Clang compiler. Several bugs fixes and enhancements are also found
  in this release; qexisting users are encouraged to upgrade.

Version 0.20.1 (2018-08-08)
===========================

  Minor changes at ZBar, in order to adapt to modern distributions
  with are removing /usr/bin/python in favor of just using python2.
  Also, updated some python2 scripts to work with modern distros,
  where the Image module is now inside PIL.

Version 0.20 (2017-04-11)
=========================

  As upstream didn't have any version since 2009, created a ZBar fork at
  linuxtv.org. This release improves a lot V4L2 support, by using libv4l2
  to handle formats that are alien to ZBar, making it compatible with a lot
  more webcam models. Qt support was also updated, making it compatible
  with Qt5. ZBar now have two other GUI applications (zbarcam-qt and
  zbarcam-gtk). With zbarcam-qt, it is now possible to adjust the camera
  controls, making easier to read barcodes using a camera.

Version 0.10 (2009-10-23)
=========================

  ZBar goes 2D!  This release introduces support for QR Code, developed
  by our new project member, Timothy Terriberry.  Timothy is an image
  processing master, providing this sophisticated functionality using
  only integer arithmetic and maintaining an extremely small image
  size.  Feel free to drop in on #zbar over at freenode to welcome
  Timothy (aka derf) to the project and congratulate him on his awesome
  work.

  Several bugs fixes and enhancements are also found in this release;
  existing users are encouraged to upgrade.

Version 0.9 (2009-08-31)
========================

  Introducing ZBar for Windows!  So far we only have straight ports of
  the command line applications, but work on a cross-platform GUI has
  already begun.  This release also has a few scanner/decoder
  enhancements and fixes several bugs.  Many internal changes are
  represented, so please open a support request if you experience any
  problems.

Version 0.8 (2009-06-05)
========================

  This is a bugfix release just to clean up a few issues found in the
  last release.  Existing users are encouraged to upgrade to pick up
  these fixes.

Version 0.7 (2009-04-21)
========================

  Welcome to ZBar!  In addition to finalizing the project name change,
  this release adds new bindings for Python and fixes a few bugs with
  the Perl interface.  The decoder also has several new features and
  addresses missing checks that will improve reliability with
  excessively noisy images.

Version 0.6 (2009-02-28)
========================

  This release fixes many bugs and adds several improvements suggested
  by users:  support for decoding UPC-E is finished.  zebraimg is
  now able to scan all pages of a document (such as PDF or TIFF) and
  the new XML output includes the page where each barcode is found.
  Camera support has been significantly improved, including the
  addition of JPEG image formats.  Perl bindings make it even easier
  to write a document or video scanning application.  Finally, we are
  now able to offer initial support for building the base library for
  Windows!

Version 0.5 (2008-07-25)
========================

  Introducing zebra widgets!  Prioritized by popular demand, this
  release includes fully functional barcode scanning widgets for GTK,
  PyGTK, and Qt.  Application developers may now seamlessly integrate
  barcode reader support with their user interface.

  This release also fixes many bugs; existing users are encouraged to
  upgrade.

Version 0.4 (2008-05-31)
========================

  new support for EAN-8, Code 39 and Interleaved 2 of 5!
  this release also introduces the long awaited decoder configuration
  options and fixes several bugs

Version 0.3 (2008-02-25)
========================

  this is a beta release of the enhanced internal architecture.
  support has been added for version 2 of the video4linux API and many
  more image formats.  several other feature enhancements and bug
  fixes are also included.  image scanning is slightly improved for
  some images, but the base scan/decode function is relatively
  untouched.  significant new code is represented in this release
  - all bug reports are welcome and will be addressed promptly!

Version 0.2 (2007-05-16)
========================

  this release introduces significant enhancements, bug fixes and new
  features!  basic EAN-13/UPC-A reading has been improved and should
  now be much more reliable.  by popular request, new support has been
  added for decoding Code 128.  additionally, the build process was
  improved and there is even basic documentation for the application
  commands

Version 0.1 (2007-03-24)
========================

  first official Zebra release!
  supports basic scanning and decoding of EAN-13 and UPC-A symbols
  using a webcam (zebracam) or from stored image files (zebraimg).
  still need to add support for addons and EAN-8/UPC-E
