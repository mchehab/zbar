pygtk_dep = dependency('pygtk-2.0', required: false)
if pygtk_dep.found()
  codegendir = pygtk_dep.get_variable(pkgconfig: 'codegendir')
  defsdir = pygtk_dep.get_variable(pkgconfig: 'defsdir')
  h2def = codegendir / 'h2def.py'
  codegen = find_program('pygobject-codegen-2.0',
                         'pygtk-codegen-2.0', 'pygtk-codegen')
  pygtk_defs = custom_target(
    'zbarpygtk-defs',
    input: '../include/zbar/zbargtk.h',
    output: 'zbarpygtk.defs',
    command: [py, files('../tools/pygtk_defs.py'), h2def, '@INPUT@'],
    capture: true,
  )
  pygtk_source = custom_target(
    'zbarpygtk-source',
    input: [pygtk_defs, 'zbarpygtk.override'],
    output: 'zbarpygtk.c',
    command: [codegen, '--prefix', 'zbarpygtk',
              '--register', defsdir / 'gdk-types.defs',
              '--override', '@INPUT1@', '@INPUT0@'],
    capture: true,
  )
  py.extension_module(
    'zbarpygtk', ['zbarpygtkmodule.c', pygtk_source],
    dependencies: [zbar_dep, python_dep, pygtk_dep, gtk_dep],
    link_with: gtk_lib,
    install: true,
  )
endif
