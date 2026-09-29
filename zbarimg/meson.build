zbarimg_sources = files('zbarimg.c')
if is_windows
  zbarimg_sources += windows.compile_resources('zbarimg.rc',
                                               include_directories: inc)
endif
zbarimg = executable(
  'zbarimg', zbarimg_sources,
  dependencies: [zbar_dep, magick_dep, intl_dep],
  install: true,
)
