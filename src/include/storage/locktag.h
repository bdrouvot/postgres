/*-------------------------------------------------------------------------
 *
 * locktag.h
 *	  POSTGRES LOCKTAG to look up a LOCK item in the lock hashtable.
 *
 *
 * Portions Copyright (c) 1996-2026, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/storage/locktag.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef LOCKTAG_H_
#define LOCKTAG_H_

/*
 * LOCKTAG is the key information needed to look up a LOCK item in the
 * lock hashtable.  A LOCKTAG value uniquely identifies a lockable object.
 *
 * The LockTagType enum defines the different kinds of objects we can lock.
 * We can handle up to 256 different LockTagTypes.
 */
typedef enum LockTagType
{
	LOCKTAG_RELATION,			/* whole relation */
	LOCKTAG_RELATION_EXTEND,	/* the right to extend a relation */
	LOCKTAG_DATABASE_FROZEN_IDS,	/* pg_database.datfrozenxid */
	LOCKTAG_PAGE,				/* one page of a relation */
	LOCKTAG_TUPLE,				/* one physical tuple */
	LOCKTAG_TRANSACTION,		/* transaction (for waiting for xact done) */
	LOCKTAG_VIRTUALTRANSACTION, /* virtual transaction (ditto) */
	LOCKTAG_SPECULATIVE_TOKEN,	/* speculative insertion Xid and token */
	LOCKTAG_OBJECT,				/* non-relation database object */
	LOCKTAG_USERLOCK,			/* reserved for old contrib/userlock code */
	LOCKTAG_ADVISORY,			/* advisory user locks */
	LOCKTAG_APPLY_TRANSACTION,	/* transaction being applied on a logical
								 * replication subscriber */
} LockTagType;

#define LOCKTAG_LAST_TYPE	LOCKTAG_APPLY_TRANSACTION

extern PGDLLIMPORT const char *const LockTagTypeNames[];

/*
 * The LOCKTAG struct is defined with malice aforethought to fit into 16
 * bytes with no padding.  Note that this would need adjustment if we were
 * to widen Oid, BlockNumber, or TransactionId to more than 32 bits.
 *
 * We include lockmethodid in the locktag so that a single hash table in
 * shared memory can store locks of different lockmethods.
 */
typedef struct LOCKTAG
{
	uint32		locktag_field1; /* a 32-bit ID field */
	uint32		locktag_field2; /* a 32-bit ID field */
	uint32		locktag_field3; /* a 32-bit ID field */
	uint16		locktag_field4; /* a 16-bit ID field */
	uint8		locktag_type;	/* see enum LockTagType */
	uint8		locktag_lockmethodid;	/* lockmethod indicator */
} LOCKTAG;

#endif							/* LOCKTAG_H_ */
