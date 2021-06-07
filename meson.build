project('roll', 'c',
  version : '0.1',
  default_options : ['warning_level=3'])

add_project_arguments('-march=native', language: 'c')

cc = meson.get_compiler('c')

if cc.get_id() == 'clang'
  add_project_arguments(
      '-Wno-nullability-extension', # We guard this with an ifdef so it's fine.
      '-Wno-gnu-auto-type',
      '-Wno-gnu-empty-initializer',
      '-Wno-gnu-statement-expression',
      '-Wno-gnu-case-range',
      '-Wno-extra-semi',
      '-Wno-sign-compare',
      '-Wno-language-extension-token',
      '-Wno-gnu-empty-struct',
      language : 'c')
endif

executable('roll',
           'roll.c',
           install : true)
