project(
  'zbar', 'c', 'cpp',
  version: '0.23.93',
  meson_version: '>=0.61.0',
  default_options: [
    'c_std=gnu99',
    'cpp_std=gnu++11',
    'warning_level=1',
    'default_library=both',
    'libdir=lib',
  ],
)

cc = meson.get_compiler('c')
pkg = import('pkgconfig')
fs = import('fs')
is_windows = host_machine.system() == 'windows'
inc = include_directories('.', 'include')
# Match the Autotools warning flags used by AM_CFLAGS and AM_CXXFLAGS.
add_project_arguments('-Wno-parentheses', language: ['c', 'cpp'])
if is_windows
  add_project_arguments('-D_WIN32_WINNT=0x0500', language: ['c', 'cpp'])
endif
conf = configuration_data()
conf.set_quoted('PACKAGE', 'zbar')
conf.set_quoted('PACKAGE_VERSION', meson.project_version())
conf.set_quoted('LOCALEDIR', get_option('prefix') / get_option('localedir'))
ver = meson.project_version().split('.')
conf.set('ZBAR_VERSION_MAJOR', ver[0].to_int())
conf.set('ZBAR_VERSION_MINOR', ver[1].to_int())
conf.set('ZBAR_VERSION_PATCH', ver[2].to_int())
conf.set('LIB_VERSION_MAJOR', 0)
conf.set('LIB_VERSION_MINOR', 3)
conf.set('LIB_VERSION_REVISION', 0)

headers = [
  'assert.h',
  'errno.h',
  'fcntl.h',
  'features.h',
  'float.h',
  'inttypes.h',
  'limits.h',
  'locale.h',
  'poll.h',
  'stddef.h',
  'stdint.h',
  'stdlib.h',
  'string.h',
  'sys/ioctl.h',
  'sys/ipc.h',
  'sys/mman.h',
  'sys/shm.h',
  'sys/stat.h',
  'sys/sysmacros.h',
  'sys/time.h',
  'sys/times.h',
  'sys/types.h',
  'time.h',
  'unistd.h',
]
foreach header : headers
  conf.set('HAVE_' + header.to_upper().underscorify(),
           cc.has_header(header) ? 1 : false)
endforeach

conf.set('MAJOR_IN_SYSMACROS',
         cc.has_header('sys/sysmacros.h') ? 1 : false)

# Select the image backend before generating the configuration header.
magick_dep = disabler()
magick_found = false
if not get_option('imagemagick').disabled()
  if get_option('graphicsmagick').enabled()
    magick_dep = dependency('GraphicsMagickWand', required: true)
    conf.set('HAVE_GRAPHICSMAGICK', 1)
  else
    magick_dep = dependency('MagickWand', required: false)
    if magick_dep.found()
      conf.set('HAVE_IMAGEMAGICK', 1)
      conf.set('HAVE_IMAGEMAGICK7',
               magick_dep.version().version_compare('>=7') ? 1 : false)
    elif get_option('graphicsmagick').auto()
      magick_dep = dependency('GraphicsMagickWand', required: false)
      if magick_dep.found()
        conf.set('HAVE_GRAPHICSMAGICK', 1)
      endif
    endif
  endif
  magick_found = magick_dep.found()
  if get_option('imagemagick').enabled() and not magick_found
    error('ImageMagick or GraphicsMagick development files are required')
  endif
endif

intl_dep = dependency('intl', required: get_option('nls'))
conf.set('ENABLE_NLS', intl_dep.found() ? 1 : false)
if intl_dep.found()
  subdir('po')
endif

subdir('zbar')
configure_file(output: 'config.h', configuration: conf)
subdir('include')

gtk_found = false
qt_found = false
python_found = false
java_found = false
if get_option('gtk') != 'no'
  subdir('gtk')
endif
if get_option('qt')
  subdir('qt')
endif
if get_option('python') != 'no'
  subdir('python')
endif
if not get_option('java').disabled()
  subdir('java')
endif
if not get_option('npapi').disabled()
  subdir('plugin')
endif
if magick_found
  subdir('zbarimg')
endif
if get_option('video')
  subdir('zbarcam')
endif
subdir('doc')
if get_option('build_tests')
  subdir('test')
endif

install_data(
  'COPYING', 'HACKING.md', 'INSTALL.md', 'LICENSE.md',
  'NEWS.md', 'README.md', 'TODO.md',
  install_dir: get_option('datadir') / 'doc/zbar',
)
if is_windows
  install_data('python/test/barcode.png', 'examples/scan_image.cpp',
               'examples/scan_image.vcproj',
               install_dir: get_option('datadir') / 'zbar')
  install_data('README-windows.md',
               install_dir: get_option('datadir') / 'doc/zbar')
endif
spec_conf = configuration_data()
spec_conf.set('VERSION', meson.project_version())
configure_file(input: 'zbar.spec.in', output: 'zbar.spec',
               configuration: spec_conf)

summary(
  {
    'Image scanner': magick_found,
    'Video scanner': get_option('video'),
    'GTK': gtk_found,
    'Qt': qt_found,
    'Python': python_found,
    'Java': java_found,
    'D-Bus': dbus_dep.found(),
    'Translations': intl_dep.found(),
  },
  bool_yn: true,

  section: 'Features'
)
