project('roll', 'c',
  version : '1.0',
  default_options : ['warning_level=3'])

cc = meson.get_compiler('c')

if cc.get_id() == 'clang'
  add_project_arguments(
      '-Wno-nullability-extension', # We guard this with an ifdef so it's fine.
      '-Wno-gnu-auto-type',
      '-Wno-gnu-empty-initializer',
      '-Wno-gnu-statement-expression',
      '-Wno-gnu-case-range',
      '-Wno-keyword-macro',
      language : 'c')
endif
if cc.get_id() == 'gcc'
  add_project_arguments(
    '-Wno-pedantic',
    language: 'c')

endif
if host_machine.system() == 'windows'
  add_project_arguments(
    '-D_CRT_SECURE_NO_DEPRECATE=1',
    language: 'c')
endif

executable('roll',
           'roll/roll.c',
           install : true)
