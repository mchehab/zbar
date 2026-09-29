doc_conf = configuration_data()
doc_conf.set('VERSION', meson.project_version())
doc_conf.set('top_srcdir', meson.project_source_root())
configure_file(input: 'doxygen.conf.in', output: 'doxygen.conf',
               configuration: doc_conf)
version_conf = configuration_data()
version_conf.set('VERSION', meson.project_version())
configure_file(input: 'version.xml.in', output: 'version.xml',
               configuration: version_conf)
configure_file(input: 'reldate.xml.in', output: 'reldate.xml',
               copy: true)
xmlto = find_program('xmlto', required: false)
if get_option('doc') and xmlto.found()
  manual = custom_target(
    'manual',
    input: 'manual.xml',
    output: [
      'zbarimg.1',
      'zbarcam.1',
      'zbarcam-gtk.1',
      'zbarcam-qt.1',
    ],
    command: [xmlto, '--skip-validation',
              '--searchpath', meson.current_build_dir(),
              '-m', meson.current_source_dir() / 'style.xsl',
              '-o', meson.current_build_dir(), 'man', '@INPUT@'],
    depend_files: files(
      'ref/zbarimg.xml', 'ref/zbarcam.xml', 'ref/commonoptions.xml',
      'ref/zbarcam-gtk.xml', 'ref/zbarcam-qt.xml',
      'style.xsl', 'version.xml.in', 'reldate.xml.in',
    ),
    install: true,
    install_dir: [
      magick_found ? get_option('mandir') / 'man1' : false,
      get_option('video') ? get_option('mandir') / 'man1' : false,
      gtk_found and get_option('video') ? get_option('mandir') / 'man1' : false,
      qt_found and get_option('video') ? get_option('mandir') / 'man1' : false,
    ],
  )
  html_manual = custom_target(
    'html-manual',
    input: 'manual.xml',
    output: 'html',
    command: [xmlto, '--skip-validation',
              '--searchpath', meson.current_build_dir(),
              '-m', meson.current_source_dir() / 'style.xsl',
              '-o', '@OUTPUT@', 'xhtml', '@INPUT@'],
    build_by_default: false,
  )
  alias_target('docs', manual)
  alias_target('html', html_manual)
endif
