Building ZBar On Linux
======================

ZBar build uses Meson and Ninja, which requires a separate directory to
generate the binary files outside the source tree.

It follows some instructions about how to install on different systems.

They assume that the build directory will be named `build`.

Ubuntu and Debian package dependencies
--------------------------------------

Install the compiler and basic build tools:

    sudo apt-get update
    sudo apt-get install build-essential meson ninja-build pkg-config gettext

Meson enables optional components when their development packages are
available. To build the common desktop components used by CI, install their
dependencies as well:

    sudo apt-get install libdbus-1-dev libjpeg-dev libmagick++-dev \
        libv4l-dev libgtk-3-dev libgirepository1.0-dev \
        gobject-introspection libqt5x11extras5-dev qtbase5-dev \
        python3-dev python3-gi default-jdk-headless xmlto xvfb

To use GTK 2 instead of GTK 3, install `libgtk2.0-dev` and select `-Dgtk=gtk2`
when configuring.


Fedora package dependencies
---------------------------

Install the build tools and development packages used for Fedora builds:

    sudo dnf install meson ninja-build dbus-devel gettext-devel \
        GraphicsMagick-devel gtk3-devel gobject-introspection-devel \
        libX11-devel libjpeg-devel libv4l-devel qt5-qtbase-devel \
        qt5-qtx11extras-devel python3-devel xmlto java-devel

Building with meson
-------------------

Once package dependencies are satisfied, building it can be done with:

    meson setup build
    meson compile -C build

To install the build:

    sudo meson install -C build

To run ZBar tests:

    meson test -C build

Notice that there are several features that can be optionally built or
excluded. See `meson_options.txt` for a list of them. For instance,
to disable video capture, you may use, instead of `meson setup`:

    meson setup build -Dvideo=false

You can see all available options by using:

    meson configure build

Meson normally installs under `/usr/local`. Set `--prefix=/usr` during setup
-to choose a different prefix, like on this example:

    meson setup build -Dgtk=no -Dpython=no -Dqt=false --prefix=/usr


Building ZBar On Windows
========================

NOTE:

This is a simplified version of what it was done in order to do the
CI builds via Github workflow. The the instructions here may be incomplete.
and/or not reflect the best way to compile ZBar on Windows.

If you find inconsistencies, feel free to submit patches improving the
building steps.

Also, please notice that the instructions here is for a minimal version,
without any bindings nor ImageMagick.

Cross-compiling on Ubuntu with MinGW-w64
-----------------------------------------

The Ubuntu GitHub Actions jobs cross-compile 32-bit Windows binaries with
MinGW-w64. Install the same build tools and Windows headers:

    sudo apt-get update
    sudo apt-get install -y meson ninja-build pkg-config \
        gcc-mingw-w64 g++-mingw-w64 mingw-w64-i686-dev \
        win-iconv-mingw-w64-dev

Create a Meson cross file named `mingw-cross.ini`:

    [binaries]
    c = 'i686-w64-mingw32-gcc'
    cpp = 'i686-w64-mingw32-g++'
    ar = 'i686-w64-mingw32-ar'
    strip = 'i686-w64-mingw32-strip'
    windres = 'i686-w64-mingw32-windres'
    pkgconfig = 'pkg-config'

    [properties]
    needs_exe_wrapper = true

    [built-in options]
    c_args = ['-I/usr/i686-w64-mingw32/include']
    c_link_args = ['-L/usr/i686-w64-mingw32/lib']

    [host_machine]
    system = 'windows'
    cpu_family = 'x86'
    cpu = 'i686'
    endian = 'little'

Configure and build with the Video for Windows backend:

    meson setup build --cross-file mingw-cross.ini \
        -Dgtk=no -Dpython=no -Dqt=false -Djava=disabled \
        -Dgir=false -Dimagemagick=disabled -Dgraphicsmagick=disabled \
        -Djpeg=disabled -Ddbus=disabled -Dnls=disabled -Dx11=disabled \
        -Dxshm=disabled -Dxv=disabled -Dbuild_tests=false -Dvideo=true
    meson compile -C build

To use DirectShow instead, add `-Ddirectshow=true` to the `meson setup`
command. Meson installs the cross-compiled files under `build`; to stage
them in a separate directory, run:

    DESTDIR="$PWD/stage" meson install -C build

Building natively on Windows with MSYS2
---------------------------------------

The Windows CI job builds with MSYS2 UCRT64. Install MSYS2 from
<https://www.msys2.org/>, open the UCRT64 terminal, and install the CI build
tools:

    pacman -Syu --needed \
        mingw-w64-ucrt-x86_64-gcc \
        mingw-w64-ucrt-x86_64-pkgconf \
        mingw-w64-ucrt-x86_64-meson \
        mingw-w64-ucrt-x86_64-ninja \
        mingw-w64-ucrt-x86_64-libiconv \
        mingw-w64-ucrt-x86_64-gettext

From the repository root, configure and build a minimal native Windows
version:

    meson setup build -Dvideo=false -Dgtk=no -Dpython=no -Dqt=false \
        -Dgir=false -Djava=disabled -Ddoc=false -Ddbus=disabled \
        -Djpeg=disabled -Dimagemagick=disabled -Dgraphicsmagick=disabled \
        -Dnls=disabled -Dx11=disabled -Dxshm=disabled -Dxv=disabled \
        -Dbuild_tests=false
    meson compile -C build

To install, run `meson install -C build` from the UCRT64 terminal.

The release workflow also builds 64-bit and 32-bit packages using the MSYS2
MINGW64 and MINGW32 environments. Open the matching terminal and set
`ARCH=x86_64` for MINGW64 or `ARCH=i686` for MINGW32. Install the tools for
that architecture:

    pacman -Syu --needed \
        mingw-w64-${ARCH}-gcc \
        mingw-w64-${ARCH}-meson \
        mingw-w64-${ARCH}-ninja \
        mingw-w64-${ARCH}-pkgconf \
        mingw-w64-${ARCH}-libiconv \
        mingw-w64-${ARCH}-gettext \
        base-devel git zip

Build and stage the release files with the same options as GitHub Actions:

    export CPPFLAGS=-D__USE_MINGW_ANSI_STDIO=1
    meson setup build --prefix=/usr \
        -Dvideo=false -Dgtk=no -Dgir=false -Dpython=no -Dqt=false \
        -Djava=disabled -Ddoc=false -Djpeg=disabled \
        -Dimagemagick=disabled -Dgraphicsmagick=disabled -Ddbus=disabled \
        -Dnls=disabled -Dx11=disabled -Dxshm=disabled -Dxv=disabled \
        -Dbuild_tests=false -Dpthread=true
    meson compile -C build
    DESTDIR="${PWD}/stage" meson install -C build
    python3 tools/stage_windows.py "${PWD}/stage" \
        --runtime-bin "${MINGW_PREFIX}/bin"
    (cd stage && zip -r ../zbar-win_${ARCH}.zip .)

Building ZBar On MacOS
======================

NOTE:

This is a simplified version of what it was done in order to do the
CI builds via Github workflow. The the instructions here may be incomplete.
and/or not reflect the best way to compile ZBar on MacOS.

If you find inconsistencies, feel free to submit patches improving the
building steps.

Also, please notice that the instructions here is for a minimal version,
without any bindings nor ImageMagick.


Install the dependencies used by the macOS GitHub Actions build:

    brew install meson ninja pkg-config libjpeg-turbo gettext

Set the include and library paths from Homebrew, then configure and build:

    export CPPFLAGS="-I$(brew --prefix)/include"
    export LDFLAGS="-L$(brew --prefix)/lib"
    meson setup build \
        -Dvideo=false \
        -Dpython=no \
        -Dgtk=no \
        -Dgir=false \
        -Dqt=false \
        -Djava=disabled \
        -Ddoc=false \
        -Ddbus=disabled \
        -Djpeg=disabled \
        -Dimagemagick=disabled \
        -Dgraphicsmagick=disabled \
        -Dnls=disabled \
        -Dx11=disabled \
        -Dxshm=disabled \
        -Dxv=disabled \
        -Dbuild_tests=false
    meson compile -C build

To install, use:

    meson install -C build
