gtk_choice = get_option('gtk')
gtk_dep = dependency('gtk+-3.0', required: gtk_choice == 'gtk3')
if gtk_choice == 'gtk2' or (gtk_choice == 'auto' and not gtk_dep.found())
  gtk_dep = dependency('gtk+-2.0', required: gtk_choice == 'gtk2')
endif
gtk_found = gtk_dep.found()
if gtk_found
  gnome = import('gnome')
  marshal = gnome.genmarshal(
    'zbarmarshal',
    sources: 'zbarmarshal.list',
    prefix: 'zbar_marshal',
  )
  gtk_args = []
  if x11_found
    gtk_args += '-DHAVE_X'
  endif
  if get_option('library_type') == 'both'
    gtk_lib = both_libraries(
      'zbargtk', ['zbargtk.c', marshal],
      dependencies: [zbar_dep, gtk_dep],
      c_args: gtk_args,
      version: '0.0.2',
      soversion: '0',
      install: true,
    )
    alias_target('zbargtk-libraries',
                 gtk_lib.get_shared_lib(), gtk_lib.get_static_lib())
  else
    if get_option('library_type') == 'static'
      gtk_lib = static_library(
        'zbargtk', ['zbargtk.c', marshal],
        dependencies: [zbar_dep, gtk_dep],
        c_args: gtk_args,
        install: true,
      )
    else
      gtk_lib = shared_library(
        'zbargtk', ['zbargtk.c', marshal],
        dependencies: [zbar_dep, gtk_dep],
        c_args: gtk_args,
        version: '0.0.2',
        soversion: '0',
        install: true,
      )
    endif
    alias_target('zbargtk-libraries', gtk_lib)
  endif
  install_headers('../include/zbar/zbargtk.h', subdir: 'zbar')
  gtk_pkg = gtk_dep.name()
  pkg.generate(gtk_lib,
    name: 'zbar-gtk',
    description: 'ZBar GTK widget',
    requires: ['zbar', gtk_pkg, 'gthread-2.0'],
  )
  gir_found = false
  if get_option('gir') and get_option('library_type') != 'static'
    gir_dep = dependency('gobject-introspection-1.0',
                         version: '>=0.6.7', required: false)
    gir_found = gir_dep.found()
    if gir_found
      gtk_api = gtk_pkg == 'gtk+-2.0' ? '2.0' : '3.0'
      gtk_gir = gnome.generate_gir(
        gtk_lib, libzbar,
        sources: ['../include/zbar/zbargtk.h', 'zbargtk.c'],
        namespace: 'ZBar',
        nsversion: '1.0',
        identifier_prefix: ['ZBar', 'zbar_'],
        symbol_prefix: 'zbar',
        includes: ['Gtk-' + gtk_api, 'Gdk-' + gtk_api],
        dependencies: [zbar_dep, gtk_dep],
        extra_args: ['--warn-all'],
        install: true,
      )
    endif
  endif
endif
