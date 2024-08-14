/*-------------------------------------------------------------------------
 *
 * pg_logicalsnapinspect.c
 *		  Functions to inspect contents of PostgreSQL logical snapshots
 *
 * Copyright (c) 2024, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *		  contrib/pg_logicalsnapinspect/pg_logicalsnapinspect.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "funcapi.h"
#include "port/pg_crc32c.h"
#include "replication/snapbuild.h"
#include "utils/array.h"
#include "utils/pg_lsn.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(pg_get_logical_snapshot_meta);
PG_FUNCTION_INFO_V1(pg_get_logical_snapshot_info);

static void ValidateSnapshotFile(XLogRecPtr lsn, SnapBuildOnDisk *ondisk,
								 const char *path);

/*
 * NOTE: For any code change or issue fix here, it is highly recommended to
 * give a thought about doing the same in SnapBuildRestore() as well.
 */

/*
 * Validate the logical snapshot file.
 */
static void
ValidateSnapshotFile(XLogRecPtr lsn, SnapBuildOnDisk *ondisk, const char *path)
{
	int			fd;
	Size		sz;
	pg_crc32c	checksum;
	MemoryContext context;

	context = AllocSetContextCreate(CurrentMemoryContext,
									"logicalsnapshot inspect context",
									ALLOCSET_DEFAULT_SIZES);

	fd = OpenTransientFile(path, O_RDONLY | PG_BINARY);

	if (fd < 0 && errno == ENOENT)
		ereport(ERROR,
				errmsg("file \"%s\" does not exist", path));
	else if (fd < 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not open file \"%s\": %m", path)));

	/* ----
	 * Make sure the snapshot had been stored safely to disk, that's normally
	 * cheap.
	 * Note that we do not need PANIC here, nobody will be able to use the
	 * slot without fsyncing, and saving it won't succeed without an fsync()
	 * either...
	 * ----
	 */
	fsync_fname(path, false);
	fsync_fname("pg_logical/snapshots", true);


	/* read statically sized portion of snapshot */
	SnapBuildRestoreContents(fd, (char *) ondisk, SnapBuildOnDiskConstantSize, path);

	if (ondisk->magic != SNAPBUILD_MAGIC)
		ereport(ERROR,
				(errcode(ERRCODE_DATA_CORRUPTED),
				 errmsg("snapbuild state file \"%s\" has wrong magic number: %u instead of %u",
						path, ondisk->magic, SNAPBUILD_MAGIC)));

	if (ondisk->version != SNAPBUILD_VERSION)
		ereport(ERROR,
				(errcode(ERRCODE_DATA_CORRUPTED),
				 errmsg("snapbuild state file \"%s\" has unsupported version: %u instead of %u",
						path, ondisk->version, SNAPBUILD_VERSION)));

	INIT_CRC32C(checksum);
	COMP_CRC32C(checksum,
				((char *) ondisk) + SnapBuildOnDiskNotChecksummedSize,
				SnapBuildOnDiskConstantSize - SnapBuildOnDiskNotChecksummedSize);

	/* read SnapBuild */
	SnapBuildRestoreContents(fd, (char *) &ondisk->builder, sizeof(SnapBuild), path);
	COMP_CRC32C(checksum, &ondisk->builder, sizeof(SnapBuild));

	ondisk->builder.context = context;

	/* restore committed xacts information */
	if (ondisk->builder.committed.xcnt > 0)
	{
		sz = sizeof(TransactionId) * ondisk->builder.committed.xcnt;
		ondisk->builder.committed.xip = MemoryContextAllocZero(ondisk->builder.context, sz);
		SnapBuildRestoreContents(fd, (char *) ondisk->builder.committed.xip, sz, path);
		COMP_CRC32C(checksum, ondisk->builder.committed.xip, sz);
	}

	/* restore catalog modifying xacts information */
	if (ondisk->builder.catchange.xcnt > 0)
	{
		sz = sizeof(TransactionId) * ondisk->builder.catchange.xcnt;
		ondisk->builder.catchange.xip = MemoryContextAllocZero(ondisk->builder.context, sz);
		SnapBuildRestoreContents(fd, (char *) ondisk->builder.catchange.xip, sz, path);
		COMP_CRC32C(checksum, ondisk->builder.catchange.xip, sz);
	}

	if (CloseTransientFile(fd) != 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not close file \"%s\": %m", path)));

	FIN_CRC32C(checksum);

	/* verify checksum of what we've read */
	if (!EQ_CRC32C(checksum, ondisk->checksum))
		ereport(ERROR,
				(errcode(ERRCODE_DATA_CORRUPTED),
				 errmsg("checksum mismatch for snapbuild state file \"%s\": is %u, should be %u",
						path, checksum, ondisk->checksum)));
}

/*
 * Retrieve the logical snapshot file metadata.
 */
Datum
pg_get_logical_snapshot_meta(PG_FUNCTION_ARGS)
{
#define PG_GET_LOGICAL_SNAPSHOT_META_COLS 3
	SnapBuildOnDisk ondisk;
	XLogRecPtr	lsn;
	HeapTuple	tuple;
	Datum		values[PG_GET_LOGICAL_SNAPSHOT_META_COLS];
	bool		nulls[PG_GET_LOGICAL_SNAPSHOT_META_COLS];
	TupleDesc	tupdesc;
	char		path[MAXPGPATH];

	lsn = PG_GETARG_LSN(0);

	sprintf(path, "pg_logical/snapshots/%X-%X.snap",
			LSN_FORMAT_ARGS(lsn));

	ValidateSnapshotFile(lsn, &ondisk, path);

	/* Build a tuple descriptor for our result type. */
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	memset(nulls, 0, sizeof(nulls));

	values[0] = Int32GetDatum(ondisk.magic);
	values[1] = Int32GetDatum(ondisk.checksum);
	values[2] = Int32GetDatum(ondisk.version);

	tuple = heap_form_tuple(tupdesc, values, nulls);

	MemoryContextReset(ondisk.builder.context);

	PG_RETURN_DATUM(HeapTupleGetDatum(tuple));

#undef PG_GET_LOGICAL_SNAPSHOT_META_COLS
}

Datum
pg_get_logical_snapshot_info(PG_FUNCTION_ARGS)
{
#define PG_GET_LOGICAL_SNAPSHOT_INFO_COLS 14
	SnapBuildOnDisk ondisk;
	XLogRecPtr	lsn;
	HeapTuple	tuple;
	Datum		values[PG_GET_LOGICAL_SNAPSHOT_INFO_COLS];
	bool		nulls[PG_GET_LOGICAL_SNAPSHOT_INFO_COLS];
	TupleDesc	tupdesc;
	char		path[MAXPGPATH];

	lsn = PG_GETARG_LSN(0);

	sprintf(path, "pg_logical/snapshots/%X-%X.snap",
			LSN_FORMAT_ARGS(lsn));

	ValidateSnapshotFile(lsn, &ondisk, path);

	/* Build a tuple descriptor for our result type. */
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	memset(nulls, 0, sizeof(nulls));

	values[0] = Int16GetDatum(ondisk.builder.state);
	values[1] = TransactionIdGetDatum(ondisk.builder.xmin);
	values[2] = TransactionIdGetDatum(ondisk.builder.xmax);
	values[3] = LSNGetDatum(ondisk.builder.start_decoding_at);
	values[4] = LSNGetDatum(ondisk.builder.two_phase_at);
	values[5] = TransactionIdGetDatum(ondisk.builder.initial_xmin_horizon);
	values[6] = BoolGetDatum(ondisk.builder.building_full_snapshot);
	values[7] = BoolGetDatum(ondisk.builder.in_slot_creation);
	values[8] = LSNGetDatum(ondisk.builder.last_serialized_snapshot);
	values[9] = TransactionIdGetDatum(ondisk.builder.next_phase_at);
	values[10] = Int64GetDatum(ondisk.builder.committed.xcnt);

	if (ondisk.builder.committed.xcnt > 0)
	{
		Datum	   *arrayelems;
		int			narrayelems;

		arrayelems = (Datum *) palloc(ondisk.builder.committed.xcnt * sizeof(Datum));
		narrayelems = 0;

		for (narrayelems = 0; narrayelems < ondisk.builder.committed.xcnt; narrayelems++)
			arrayelems[narrayelems] = Int64GetDatum((int64) ondisk.builder.committed.xip[narrayelems]);

		values[11] = PointerGetDatum(construct_array_builtin(arrayelems, narrayelems, INT8OID));
	}
	else
		nulls[11] = true;

	values[12] = Int64GetDatum(ondisk.builder.catchange.xcnt);

	if (ondisk.builder.catchange.xcnt > 0)
	{
		Datum	   *arrayelems;
		int			narrayelems;

		arrayelems = (Datum *) palloc(ondisk.builder.catchange.xcnt * sizeof(Datum));
		narrayelems = 0;

		for (narrayelems = 0; narrayelems < ondisk.builder.catchange.xcnt; narrayelems++)
			arrayelems[narrayelems] = Int64GetDatum((int64) ondisk.builder.catchange.xip[narrayelems]);

		values[13] = PointerGetDatum(construct_array_builtin(arrayelems, narrayelems, INT8OID));
	}
	else
		nulls[13] = true;

	tuple = heap_form_tuple(tupdesc, values, nulls);

	MemoryContextReset(ondisk.builder.context);

	PG_RETURN_DATUM(HeapTupleGetDatum(tuple));

#undef PG_GET_LOGICAL_SNAPSHOT_INFO_COLS
}
