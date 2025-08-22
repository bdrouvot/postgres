# src/test/modules/test_tranches/Makefile

MODULE_big = test_tranches
OBJS = \
	$(WIN32RES) \
	test_tranches.o
PGFILEDESC = "test_tranches - test code LWLock tranche management"

EXTENSION = test_tranches
DATA = test_tranches--1.0.sql

TAP_TESTS = 1

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = src/test/modules/test_tranches
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
