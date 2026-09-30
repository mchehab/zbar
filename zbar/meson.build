codes = get_option('codes').split(',')
symbologies = [
  'ean',
  'databar',
  'code128',
  'code93',
  'code39',
  'codabar',
  'i25',
  'qrcode',
  'sqcode',
  'pdf417',
]
foreach code : codes
  if code != 'all' and code != '' and not symbologies.contains(code)
    error('Unknown barcode symbology: ' + code)
  endif
endforeach
foreach code : symbologies
  conf.set('ENABLE_' + code.to_upper(),
           (codes.contains('all') or codes.contains(code)) ? 1 : 0)
endforeach

thread_dep = dependency('threads', required: get_option('pthread'))
conf.set('HAVE_LIBPTHREAD',
         get_option('pthread') and not is_windows ? 1 : false)
deps = []
if get_option('pthread')
  deps += thread_dep
endif
if not cc.has_function('clock_gettime', prefix: '#include <time.h>')
  rt_dep = cc.find_library('rt', required: false)
  if rt_dep.found()
    deps += rt_dep
  endif
endif

src = files(
  'config.c',
  'error.c',
  'symbol.c',
  'image.c',
  'convert.c',
  'processor.c',
  'processor/lock.c',
  'refcnt.c',
  'window.c',
  'video.c',
  'img_scanner.c',
  'scanner.c',
  'decoder.c',
  'misc.c',
)
if codes.contains('ean') or codes.contains('all')
  src += files('decoder/ean.c')
endif
linear_codes = [
  'databar',
  'code128',
  'code93',
  'code39',
  'codabar',
  'i25',
  'pdf417',
]
foreach code : linear_codes
  if codes.contains(code) or codes.contains('all')
    src += files('decoder/' + code + '.c')
  endif
endforeach
if codes.contains('qrcode') or codes.contains('all')
  iconv_dep = dependency('iconv', required: true)
  iconv_const_input = cc.compiles(
    '''#include <stddef.h>
       #include <iconv.h>
       void test(iconv_t cd, const char **input, size_t *left) {
         iconv(cd, input, left, NULL, NULL);
       }''',
    args: ['-Werror=incompatible-pointer-types'],
    dependencies: iconv_dep,
    name: 'iconv accepts a const input pointer',
  )
  conf.set('HAVE_ICONV_CONST_INPUT', iconv_const_input ? 1 : false)
  deps += iconv_dep
  src += files(
    'decoder/qr_finder.c',
    'qrcode/qrdec.c',
    'qrcode/qrdectxt.c',
    'qrcode/rs.c',
    'qrcode/isaac.c',
    'qrcode/bch15_5.c',
    'qrcode/binarize.c',
    'qrcode/util.c',
  )
endif
if codes.contains('sqcode') or codes.contains('all')
  src += files('sqcode.c', 'decoder/sq_finder.c')
endif

x11_dep = dependency('x11', required: get_option('x11'))
x11_found = x11_dep.found() and not is_windows
conf.set('HAVE_X', x11_found ? 1 : false)
conf.set('HAVE_X11_EXTENSIONS_XSHM_H', false)
conf.set('HAVE_X11_EXTENSIONS_XVLIB_H', false)
if is_windows
  src += files('processor/win.c', 'window/win.c', 'window/dib.c')
  deps += [cc.find_library('gdi32'), cc.find_library('winmm')]
elif x11_found
  src += files('processor/posix.c', 'processor/x.c',
               'window/x.c', 'window/ximage.c')
  deps += x11_dep
  xshm_dep = dependency('xext', required: get_option('xshm'))
  if xshm_dep.found()
    conf.set('HAVE_X11_EXTENSIONS_XSHM_H', 1)
    deps += xshm_dep
  endif
  xv_dep = dependency('xv', required: get_option('xv'))
  xv_header = xv_dep.found() and cc.has_header(
    'X11/extensions/Xvlib.h', dependencies: xv_dep,
  )
  if xv_header
    conf.set('HAVE_X11_EXTENSIONS_XVLIB_H', 1)
    src += files('window/xv.c')
    deps += xv_dep
  elif get_option('xv').enabled()
    error('XVideo was enabled, but X11/extensions/Xvlib.h is unavailable')
  endif
else
  if get_option('xshm').enabled() or get_option('xv').enabled()
    error('XShm and XVideo require X11 support')
  endif
  src += files('processor/posix.c', 'processor/null.c', 'window/null.c')
endif

if is_windows and get_option('video')
  if get_option('directshow')
    src += files('video/dshow.c')
    deps += [cc.find_library('oleaut32'), cc.find_library('ole32')]
  else
    src += files('video/vfw.c')
    deps += cc.find_library('vfw32')
  endif
elif get_option('video') and host_machine.system() == 'linux'
  video_headers = ['linux/videodev.h', 'linux/videodev2.h']
  foreach header : video_headers
    conf.set('HAVE_' + header.to_upper().underscorify(),
             cc.has_header(header) ? 1 : false)
  endforeach
  v4l1_found = cc.has_header('linux/videodev.h')
  v4l2_found = cc.has_header('linux/videodev2.h')
  if not v4l1_found and not v4l2_found
    error('Video enabled but no Video4Linux headers found')
  endif
  src += files('video/v4l.c')
  if v4l1_found
    src += files('video/v4l1.c')
  endif
  if v4l2_found
    src += files('video/v4l2.c')
  endif
  v4l_dep = dependency('libv4l2', required: false)
  conf.set('HAVE_LIBV4L2_H', v4l_dep.found() ? 1 : false)
  if v4l_dep.found()
    deps += v4l_dep
  endif
elif get_option('video')
  error('Video support requires Linux or Windows; use -Dvideo=false')
else
  src += files('video/null.c')
endif

jpeg_dep = dependency('libjpeg', required: get_option('jpeg'))
conf.set('HAVE_LIBJPEG', jpeg_dep.found() ? 1 : false)
if jpeg_dep.found()
  src += files('jpeg.c')
  deps += jpeg_dep
endif
dbus_dep = dependency('dbus-1', version: '>=1.0',
                      required: get_option('dbus'))
conf.set('HAVE_DBUS', dbus_dep.found() ? 1 : false)
if dbus_dep.found()
  deps += dbus_dep
  dbusconfdir = get_option('dbusconfdir')
  if dbusconfdir == ''
    dbusconfdir = dbus_dep.get_variable(
      pkgconfig: 'sysconfdir',
      default_value: get_option('sysconfdir'),
    )
  endif
  install_data('../dbus/org.linuxtv.Zbar.conf',
               install_dir: dbusconfdir / 'dbus-1/system.d')
endif

zbar_args = []
if get_option('directshow')
  zbar_args += '-DCOBJMACROS'
endif
if is_windows
  windows = import('windows')
  src += windows.compile_resources('libzbar.rc',
                                    include_directories: inc)
endif
zbar_link_args = []
zbar_link_depends = []
if host_machine.system() == 'linux'
  zbar_map = meson.current_source_dir() / 'zbar.map'
  zbar_link_args += '-Wl,--version-script=' + zbar_map
  zbar_link_depends += files('zbar.map')
endif
if get_option('library_type') == 'both'
  libzbar = both_libraries(
    'zbar', src,
    include_directories: inc,
    dependencies: deps,
    c_args: zbar_args,
    link_args: zbar_link_args,
    link_depends: zbar_link_depends,
    version: '0.3.0',
    soversion: '0',
    install: true,
  )
  alias_target('zbar-libraries',
               libzbar.get_shared_lib(), libzbar.get_static_lib())
else
  if get_option('library_type') == 'static'
    libzbar = static_library(
      'zbar', src,
      include_directories: inc,
      dependencies: deps,
      c_args: zbar_args,
      link_args: zbar_link_args,
      link_depends: zbar_link_depends,
      install: true,
    )
  else
    libzbar = shared_library(
      'zbar', src,
      include_directories: inc,
      dependencies: deps,
      c_args: zbar_args,
      link_args: zbar_link_args,
      link_depends: zbar_link_depends,
      version: '0.3.0',
      soversion: '0',
      install: true,
    )
  endif
  alias_target('zbar-libraries', libzbar)
endif
zbar_dep = declare_dependency(
  link_with: libzbar,
  include_directories: inc,
  dependencies: deps,
)
pkg.generate(libzbar,
  name: 'zbar',
  description: 'Barcode scanning library',
  url: 'https://github.com/mchehab/zbar',
)
