# contrib/pg_logicalsnapinspect/Makefile

MODULE_big = pg_logicalsnapinspect
OBJS = \
	$(WIN32RES) \
	pg_logicalsnapinspect.o
PGFILEDESC = "pg_logicalsnapinspect - functions to inspect logical snapshots"

EXTENSION = pg_logicalsnapinspect
DATA = pg_logicalsnapinspect--1.0.sql

EXTRA_INSTALL = contrib/test_decoding

ISOLATION = logical_snapshot_inspect

ISOLATION_OPTS = --temp-config $(top_srcdir)/contrib/pg_logicalsnapinspect/logicalsnapinspect.conf

# Disabled because these tests require "wal_level=logical", which
# some installcheck users do not have (e.g. buildfarm clients).
NO_INSTALLCHECK = 1

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/pg_logicalsnapinspect
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
