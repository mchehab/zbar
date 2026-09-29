zbarcam_sources = files('zbarcam.c')
zbarcam_args = []
zbarcam_deps = [zbar_dep, intl_dep]
if is_windows
  zbarcam_sources += windows.compile_resources('zbarcam.rc',
                                               include_directories: inc)
  if get_option('directshow')
    zbarcam_args += '-DDIRECTSHOW'
    zbarcam_deps += cc.find_library('ole32')
  endif
endif
executable(
  'zbarcam', zbarcam_sources,
  dependencies: zbarcam_deps,
  c_args: zbarcam_args,
  install: true,
)
if gtk_found and not is_windows
  executable(
    'zbarcam-gtk', ['zbarcam-gtk.c', 'scan_video.c'],
    dependencies: [zbar_dep, gtk_dep],
    link_with: gtk_lib,
    install: true,
  )
endif
if qt_found
  camera_moc = custom_target(
    'moc-zbarcam-qt',
    input: 'zbarcam-qt.cpp',
    output: 'moc_zbarcam_qt.h',
    command: [qt_moc_program, '-i', '@INPUT@', '-o', '@OUTPUT@'],
  )
  executable(
    'zbarcam-qt', ['zbarcam-qt.cpp', 'scan_video.c', camera_moc],
    dependencies: [zbar_dep, qt_dep],
    link_with: qt_lib,
    override_options: qt_options,
    install: true,
  )
endif
