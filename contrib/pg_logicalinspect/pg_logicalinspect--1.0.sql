/* contrib/pg_logicalinspect/pg_logicalinspect--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_logicalinspect" to load this file. \quit

--
-- pg_get_logical_snapshot_meta()
--
CREATE FUNCTION pg_get_logical_snapshot_meta(IN in_lsn pg_lsn,
    OUT magic int4,
    OUT checksum int4,
    OUT version int4
)
AS 'MODULE_PATHNAME', 'pg_get_logical_snapshot_meta'
LANGUAGE C STRICT PARALLEL SAFE;

REVOKE EXECUTE ON FUNCTION pg_get_logical_snapshot_meta(pg_lsn) FROM PUBLIC;
GRANT EXECUTE ON FUNCTION pg_get_logical_snapshot_meta(pg_lsn) TO pg_read_server_files;

--
-- pg_get_logical_snapshot_info()
--
CREATE FUNCTION pg_get_logical_snapshot_info(IN in_lsn pg_lsn,
    OUT state int2,
    OUT xmin xid,
    OUT xmax xid,
    OUT start_decoding_at pg_lsn,
    OUT two_phase_at pg_lsn,
    OUT initial_xmin_horizon xid,
    OUT building_full_snapshot boolean,
    OUT in_slot_creation boolean,
    OUT last_serialized_snapshot pg_lsn,
    OUT next_phase_at xid,
    OUT committed_count int8,
    OUT committed_xip xid[],
    OUT catchange_count int8,
    OUT catchange_xip xid[]
)
AS 'MODULE_PATHNAME', 'pg_get_logical_snapshot_info'
LANGUAGE C STRICT PARALLEL SAFE;

REVOKE EXECUTE ON FUNCTION pg_get_logical_snapshot_info(pg_lsn) FROM PUBLIC;
GRANT EXECUTE ON FUNCTION pg_get_logical_snapshot_info(pg_lsn) TO pg_read_server_files;
