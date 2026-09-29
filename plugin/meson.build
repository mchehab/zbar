npapi_dep = dependency('firefox-plugin', required: get_option('npapi'))
if npapi_dep.found()
  library(
    'zbarplugin', 'plugin.c',
    dependencies: [zbar_dep, npapi_dep],
    install: true,
  )
endif
