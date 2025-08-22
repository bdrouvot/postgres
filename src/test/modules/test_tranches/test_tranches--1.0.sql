-- test_tranches--1.0.sql

CREATE FUNCTION test_tranches_new(bigint)
RETURNS void
AS 'MODULE_PATHNAME', 'test_tranches_new'
LANGUAGE C STRICT;

CREATE FUNCTION test_tranches_lookup(int)
RETURNS text
AS 'MODULE_PATHNAME', 'test_tranches_lookup'
LANGUAGE C STRICT;

CREATE FUNCTION test_tranches_get_named_lwlock(text, int)
RETURNS int
AS 'MODULE_PATHNAME', 'test_tranches_get_named_lwlock'
LANGUAGE C STRICT;

CREATE FUNCTION test_tranches_get_first_user_defined()
RETURNS int
AS 'MODULE_PATHNAME', 'test_tranches_get_first_user_defined'
LANGUAGE C STRICT;

CREATE FUNCTION test_tranches_lwlock_initialize(int)
RETURNS void
AS 'MODULE_PATHNAME', 'test_tranches_lwlock_initialize'
LANGUAGE C STRICT;

/*
 * Function is CALLED ON NULL INPUT to allow NULL input
 * for testing
 */
CREATE FUNCTION test_tranches_new_tranche(text)
RETURNS int
AS 'MODULE_PATHNAME', 'test_tranches_new_tranche'
LANGUAGE C;