# Copyright (c) 2024-2025, PostgreSQL Global Development Group

test_tranches_sources = files(
  'test_tranches.c',
)

if host_system == 'windows'
  test_tranches_sources += rc_lib_gen.process(win32ver_rc, extra_args: [
    '--NAME', 'test_tranches',
    '--FILEDESC', 'test_tranches - test code LWLock tranche management',])
endif

test_tranches = shared_module('test_tranches',
  test_tranches_sources,
  kwargs: pg_test_mod_args,
)
test_install_libs += test_tranches

test_install_data += files(
  'test_tranches.control',
  'test_tranches--1.0.sql',
)

tests += {
  'name': 'test_tranches',
  'sd': meson.current_source_dir(),
  'bd': meson.current_build_dir(),
  'tap': {
    'tests': [
      't/001_test_tranches.pl',
    ],
  },
}