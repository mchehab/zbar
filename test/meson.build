test_decode = executable('test_decode', 'test_decode.c',
                          dependencies: zbar_dep)
decoder_codes = ['ean', 'databar', 'code128', 'code93', 'code39',
                 'codabar', 'i25']
full_decoder = codes.contains('all')
if not full_decoder
  full_decoder = true
  foreach code : decoder_codes
    full_decoder = full_decoder and codes.contains(code)
  endforeach
endif
if full_decoder
  test('decoder', test_decode, args: ['-q'])
  test('decoder-regression', test_decode, args: ['-q', '-n', '100000'],
       suite: 'regression', timeout: 300)
endif

image_sources = files('test_images.c')
test_convert = executable('test_convert', ['test_convert.c', image_sources],
                           dependencies: zbar_dep)
test('conversion', test_convert, suite: 'other')
executable('test_proc', ['test_proc.c', image_sources],
            dependencies: zbar_dep)
executable('test_cpp', 'test_cpp.cpp', dependencies: zbar_dep)
cpp_image_test = executable('test_cpp_img',
                            ['test_cpp_img.cpp', image_sources],
                            dependencies: zbar_dep)
if x11_found
  test('cpp-images', cpp_image_test, suite: 'display')
endif
if get_option('video')
  executable('test_video', ['test_video.c', image_sources],
              dependencies: zbar_dep)
endif
if jpeg_dep.found()
  jpeg_test = executable('test_jpeg', 'test_jpeg.c', dependencies: zbar_dep)
  if x11_found
    test('jpeg', jpeg_test, args: ['-q'], suite: 'display')
  endif
endif
if dbus_dep.found()
  executable('test_dbus', 'test_dbus.c',
              dependencies: [dbus_dep, zbar_dep])
endif

shell_conf = configuration_data()
shell_conf.set('abs_top_srcdir', meson.project_source_root())
shell_conf.set('abs_top_builddir', meson.project_build_root())
foreach code : symbologies
  shell_conf.set('ENABLE_' + code.to_upper(), conf.get('ENABLE_' +
                                                         code.to_upper()))
endforeach
image_script = configure_file(
  input: 'test_examples.sh.in', output: 'test_examples.sh',
  configuration: shell_conf,
)
configure_file(input: 'check_dbus.sh.in', output: 'check_dbus.sh',
               configuration: shell_conf)
bash = find_program('bash', required: false)
if magick_found and bash.found() and not meson.is_cross_build()
  test('images', bash, args: [image_script])
  image_python = import('python').find_installation('python3')
  image_env = environment()
  image_env.set('ZBARIMG', zbarimg.full_path())
  if codes.contains('ean') or codes.contains('all')
    test('image-xml', image_python, args: files('barcodetest.py'),
         env: image_env)
  endif
endif

if python_found and not meson.is_cross_build()
  pil = run_command(py, '-c', 'from PIL import Image', check: false)
  if pil.returncode() == 0 and (codes.contains('ean') or codes.contains('all'))
    python_env = environment()
    python_env.set('PYTHONPATH', meson.project_build_root() / 'python')
    test('python', py,
         args: [files('test_python.py'),
                meson.project_source_root() / 'examples/ean-13.png',
                '9789876543217'],
         env: python_env,
         depends: python_extension)
  endif
endif

if gtk_found and gir_found and not meson.is_cross_build()
  gi_python = import('python').find_installation('python3', required: false)
  if gi_python.found()
    gi_available = run_command(gi_python, '-c', 'import gi', check: false)
    if gi_available.returncode() == 0
      gir_env = environment()
      gir_env.set('GI_TYPELIB_PATH', meson.project_build_root() / 'gtk')
      gir_env.set('LD_LIBRARY_PATH',
                  meson.project_build_root() / 'gtk' + ':' +
                  meson.project_build_root() / 'zbar')
      test('introspection', gi_python,
           args: ['-c', 'import gi; gi.require_version("ZBar", "1.0"); '
                        + 'from gi.repository import ZBar; '
                        + 'assert ZBar.Gtk.__gtype__'],
           env: gir_env,
           depends: gtk_gir)
    endif
  endif
endif

if java_found and not meson.is_cross_build()
  junit = '/usr/share/java/junit4.jar'
  if fs.exists(junit)
    java_tests = files('test_java.py')
    test('java', python_build,
         args: [java_tests, javac.full_path(), java.full_path(),
                meson.project_build_root() / 'java/zbar.jar',
                java_lib.full_path(), junit,
                files('../java/test/TestImage.java',
                      '../java/test/TestImageScanner.java',
                      '../java/test/TestScanImage.java')],
         depends: [java_artifacts, java_lib])
  endif
endif
