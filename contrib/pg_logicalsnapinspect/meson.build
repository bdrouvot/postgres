# Copyright (c) 2024, PostgreSQL Global Development Group

pg_logicalsnapinspect_sources = files('pg_logicalsnapinspect.c')

if host_system == 'windows'
  pg_logicalsnapinspect_sources += rc_lib_gen.process(win32ver_rc, extra_args: [
    '--NAME', 'pg_logicalsnapinspect',
    '--FILEDESC', 'pg_logicalsnapinspect - functions to inspect contents of logical snapshots',])
endif

pg_logicalsnapinspect = shared_module('pg_logicalsnapinspect',
  pg_logicalsnapinspect_sources,
  kwargs: contrib_mod_args + {
      'dependencies': contrib_mod_args['dependencies'],
  },
)
contrib_targets += pg_logicalsnapinspect

install_data(
  'pg_logicalsnapinspect.control',
  'pg_logicalsnapinspect--1.0.sql',
  kwargs: contrib_data_args,
)

tests += {
  'name': 'pg_logicalsnapinspect',
  'sd': meson.current_source_dir(),
  'bd': meson.current_build_dir(),
  'isolation': {
    'specs': [
      'logical_snapshot_inspect',
    ],
    'regress_args': [
      '--temp-config', files('logicalsnapinspect.conf'),
    ],
    # see above
    'runningcheck': false,
  },
}
