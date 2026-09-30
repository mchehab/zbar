qt_version = get_option('qt6') ? '6' : '5'
qt_modules = ['Core', 'Gui', 'Widgets']
if get_option('qt6')
  # Qt 6.2 distributions may provide CMake metadata without pkg-config files.
  qt_dependencies = []
  foreach module : qt_modules
    qt_dependencies += dependency(
      'Qt6' + module,
      method: 'cmake',
      modules: 'Qt6::' + module,
      version: '>=6.2',
    )
  endforeach
  qt_dep = declare_dependency(dependencies: qt_dependencies)
  qmake6 = find_program('qmake6', 'qmake-qt6')
  qt_moc_dir = run_command(qmake6, '-query', 'QT_HOST_LIBEXECS',
                           check: true).stdout().strip()
else
  qt_modules += 'X11Extras'
  qt_dep = dependency('qt5', modules: qt_modules, required: false)
  if qt_dep.found()
    qt_moc_dir = qt_dep.get_variable(pkgconfig: 'host_bins',
                                     configtool: 'QT_HOST_BINS')
  endif
endif
qt_found = qt_dep.found()
if qt_found
  qt_moc_program = find_program(qt_moc_dir / 'moc',
                                'moc-qt' + qt_version, 'moc')
  qt_moc_headers = files('../include/zbar/QZBar.h', 'QZBarThread.h')
  qt_moc = []
  foreach header : qt_moc_headers
    qt_moc += custom_target(
      'moc-' + fs.stem(header),
      input: header,
      output: 'moc_' + fs.stem(header) + '.cpp',
      command: [qt_moc_program, '@INPUT@', '-o', '@OUTPUT@'],
    )
  endforeach
  qt_sources = ['QZBar.cpp', 'QZBarThread.cpp', 'QZBarRenderer.cpp', qt_moc]
  qt_options = get_option('qt6') ? ['cpp_std=gnu++17'] : []
  if get_option('static_qt')
    qt_lib = static_library(
      'zbarqt', qt_sources,
      dependencies: [zbar_dep, qt_dep],
      override_options: qt_options,
      install: true,
    )
  else
    qt_lib = library(
      'zbarqt', qt_sources,
      dependencies: [zbar_dep, qt_dep],
      override_options: qt_options,
      version: '0.1.2',
      soversion: '0',
      install: true,
    )
  endif
  install_headers('../include/zbar/QZBar.h',
                   '../include/zbar/QZBarImage.h', subdir: 'zbar')
  qt_packages = []
  foreach module : qt_modules
    qt_packages += 'Qt' + qt_version + module
  endforeach
  qt_requires = ['zbar']
  qt_pc_libraries = []
  if get_option('qt6')
    qt_pc_libraries += qt_dependencies
  else
    qt_requires += qt_packages
  endif
  pkg.generate(qt_lib,
    name: 'zbar-qt',
    description: 'ZBar Qt widget',
    requires: qt_requires,
    libraries: qt_pc_libraries,
  )
endif
