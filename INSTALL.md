Building ZBar with Meson
========================

Install Meson, Ninja, a C/C++ compiler, pkg-config, and the development
packages for the features you want. Optional features can be disabled with
Meson options; run `meson configure builddir` to list available options.

Configure and build in a separate directory:

    meson setup builddir
    meson compile -C builddir
    meson install -C builddir

For example, to disable video and select GTK 3 and Python 3:

    meson setup builddir -Dvideo=false -Dgtk=gtk3 -Dpython=python3

Meson feature options accept `enabled`, `disabled`, or `auto` where applicable.
Use `--prefix=/usr` during setup to change the installation prefix.
