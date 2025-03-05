# Test the pg_logicalinspect functions: that needs some permutation to
# ensure that we are creating at least one snapshot that contains ongoing and
# committed catalogs changes.
setup
{
    DROP TABLE IF EXISTS tbl1;
    CREATE TABLE tbl1 (val1 integer, val2 integer);
    CREATE EXTENSION pg_logicalinspect;
}

teardown
{
    DROP TABLE tbl1;
    DROP TABLE tbl2;
    SELECT 'stop' FROM pg_drop_replication_slot('isolation_slot');
    DROP EXTENSION pg_logicalinspect;
}

session "s0"
setup { SET synchronous_commit=on; }
step "s0_init" { SELECT 'init' FROM pg_create_logical_replication_slot('isolation_slot', 'test_decoding'); }
step "s0_begin" { BEGIN; }
step "s0_savepoint" { SAVEPOINT sp1; }
step "s0_truncate" { TRUNCATE tbl1; }
step "s0_commit" { COMMIT; }

session "s1"
setup { SET synchronous_commit=on; }
step "s1_checkpoint" { CHECKPOINT; }
step "s1_create_table" { CREATE TABLE tbl2 (val1 integer, val2 integer); }
step "s1_get_changes" { SELECT data FROM pg_logical_slot_get_changes('isolation_slot', NULL, NULL, 'skip-empty-xacts', '1', 'include-xids', '0'); }
step "s1_get_logical_snapshot_meta" { SELECT COUNT(meta.*) > 0 AS has_meta from pg_ls_logicalsnapdir(), pg_get_logical_snapshot_meta(name) as meta; }
step "s1_get_logical_snapshot_info" { SELECT count(*) > 0 as has_info FROM pg_ls_logicalsnapdir(), pg_get_logical_snapshot_info(name) AS info where info.catchange_count >= 2 and array_length(info.catchange_xip,1) >= 2 and info.committed_count >= 1 and array_length(info.committed_xip,1) >= 1; }

# s0 does not commit until the end of the test. This is needed to ensure that
# a checkpoint will not remove any snapshots. s0 produces 2 ongoing catalog changes
# (the truncate and its parent transaction). s1 produces a committed catalog change.
# So that the get_changes produces (at least) one snapshot that contains 2
# ongoing catalog changes and the committed catalog change.
permutation "s0_init" "s0_begin" "s0_savepoint" "s0_truncate" "s1_create_table" "s1_checkpoint" "s1_get_changes" "s1_get_logical_snapshot_info" "s1_get_logical_snapshot_meta" "s0_commit"
