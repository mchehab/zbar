javac = find_program('javac', required: get_option('java'))
jar = find_program('jar', required: get_option('java'))
java = find_program('java', required: get_option('java'))
if javac.found() and jar.found() and java.found()
  if meson.is_cross_build()
    if get_option('java').enabled()
      error('Cross compiling JNI requires a target JDK; disable Java bindings')
    endif
    subdir_done()
  endif
  # Resolve the JDK used by javac rather than mixing different installations.
  python_build = import('python').find_installation('python3')
  java_home = run_command(python_build,
    '-c', 'import os,sys; print(os.path.dirname(os.path.dirname('
          + 'os.path.realpath(sys.argv[1]))))',
    javac.full_path(), check: true,
  ).stdout().strip()
  if is_windows
    jni_platform = 'win32'
  elif host_machine.system() == 'darwin'
    jni_platform = 'darwin'
  else
    jni_platform = 'linux'
  endif
  java_includes = include_directories(
    java_home / 'include', java_home / 'include' / jni_platform,
  )
  java_found = cc.has_header('jni.h',
                            include_directories: java_includes)
  if java_found
    java_sources = files(
      'net/sourceforge/zbar/Config.java',
      'net/sourceforge/zbar/Modifier.java',
      'net/sourceforge/zbar/Orientation.java',
      'net/sourceforge/zbar/NativeCleanup.java',
      'net/sourceforge/zbar/Symbol.java',
      'net/sourceforge/zbar/SymbolIterator.java',
      'net/sourceforge/zbar/SymbolSet.java',
      'net/sourceforge/zbar/Image.java',
      'net/sourceforge/zbar/ImageScanner.java',
    )
    java_dir = get_option('datadir') / 'zbar/lib'
    java_artifacts = custom_target(
      'zbar-java',
      input: java_sources,
      output: ['zbar.jar', 'zbarjni.h'],
      command: [python_build, files('../tools/build_java.py'),
                javac, jar, '@OUTPUT0@', '@OUTPUT1@', '@INPUT@'],
      install: true,
      install_dir: [java_dir, false],
    )
    java_lib = shared_module(
      'zbarjni', ['zbarjni.c', java_artifacts[1]],
      include_directories: java_includes,
      dependencies: zbar_dep,
      install: true,
      install_dir: java_dir,
    )
  elif get_option('java').enabled()
    error('Java JNI headers are required')
  endif
endif
