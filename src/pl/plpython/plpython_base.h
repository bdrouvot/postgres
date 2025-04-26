/*-------------------------------------------------------------------------
 *
 * plpython_base.h - Python as a procedural language for PostgreSQL
 *
 * Portions Copyright (c) 1996-2025, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/pl/plpython/plpython_base.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PLPYTHON_BASE_H
#define PLPYTHON_BASE_H

/* postgres.h needs to be included before Python.h, as usual */
#if !defined(POSTGRES_H)
#error postgres.h must be included before plpython_base.h
#elif defined(Py_PYTHON_H)
#error Python.h must be included via plpython_base.h
#endif

/*
 * Enable Python Limited API
 *
 * XXX currently not enabled on MSVC because of build failures
 */
#if !defined(_MSC_VER)
#define Py_LIMITED_API 0x03020000
#endif

/*
 * Pull in Python headers via a wrapper header, to control the scope of
 * the system_header pragma therein.
 */
#include "plpython_system.h"

/* define our text domain for translations */
#undef TEXTDOMAIN
#define TEXTDOMAIN PG_TEXTDOMAIN("plpython")

#endif							/* PLPYTHON_BASE_H */
