/* contrib/pg_buffercache/pg_buffercache--1.6--1.7.sql */

-- complain if script is sourced in psql, rather than via ALTER EXTENSION
\echo Use "ALTER EXTENSION pg_buffercache UPDATE TO '1.7'" to load this file. \quit

-- Register the new function with boolean parameter
-- This function is the core implementation for both OS pages and NUMA queries
CREATE FUNCTION pg_buffercache_os_pages(IN include_numa boolean,
    OUT bufferid integer,
    OUT os_page_num bigint,
    OUT numa_node integer)
RETURNS SETOF record
AS 'MODULE_PATHNAME', 'pg_buffercache_os_pages'
LANGUAGE C PARALLEL SAFE;

-- Create a view for convenient access.
CREATE VIEW pg_buffercache_os_pages AS
    SELECT bufferid, os_page_num
    FROM pg_buffercache_os_pages(false);

DROP VIEW pg_buffercache_numa;

-- Create a view for convenient access.
CREATE VIEW pg_buffercache_numa AS
    SELECT bufferid, os_page_num, numa_node
    FROM pg_buffercache_os_pages(true);

REVOKE ALL ON FUNCTION pg_buffercache_os_pages(boolean) FROM PUBLIC;
REVOKE ALL ON pg_buffercache_os_pages FROM PUBLIC;
REVOKE ALL ON pg_buffercache_numa FROM PUBLIC;

GRANT EXECUTE ON FUNCTION pg_buffercache_os_pages(boolean) TO pg_monitor;
GRANT SELECT ON pg_buffercache_os_pages TO pg_monitor;
GRANT SELECT ON pg_buffercache_numa TO pg_monitor;
