python_mod = import('python')
python_choice = get_option('python')
python_name = python_choice == 'auto' ? 'python3' : python_choice
py = python_mod.find_installation(python_name,
                                 required: python_choice != 'auto')
if not py.found() and python_choice == 'auto'
  py = python_mod.find_installation('python2', required: false)
endif
if py.found()
  python_dep = py.dependency(required: python_choice != 'auto')
  python_found = python_dep.found()
  if python_found
    python_sources = files(
      'zbarmodule.c',
      'enum.c',
      'exception.c',
      'symbol.c',
      'symbolset.c',
      'symboliter.c',
      'image.c',
      'processor.c',
      'imagescanner.c',
      'decoder.c',
      'scanner.c',
    )
    python_extension = py.extension_module(
      'zbar', python_sources,
      dependencies: [zbar_dep, python_dep],
      install: true,
    )
    if py.language_version().version_compare('<3') and gtk_found
      subdir('../pygtk')
    endif
  endif
endif
