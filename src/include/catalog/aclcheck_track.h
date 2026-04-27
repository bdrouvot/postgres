/*-------------------------------------------------------------------------
 *
 * aclcheck_track.h
 *	  Instrumentation to detect permission check before lock via Assert in
 *	  recordMultipleDependencies() and changeDependencyFor().
 *
 *	  Only active in USE_ASSERT_CHECKING builds.
 *
 * Portions Copyright (c) 1996-2026, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/catalog/aclcheck_track.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef ACLCHECK_TRACK_H
#define ACLCHECK_TRACK_H

#ifdef USE_ASSERT_CHECKING

#include "catalog/pg_namespace.h"

#define ACLCHECK_TRACK_MAX 1024

typedef struct AclCheckEntry
{
	Oid			classId;
	Oid			objectId;
} AclCheckEntry;

extern AclCheckEntry aclcheck_tracked[];
extern int	aclcheck_tracked_count;

extern void aclcheck_track_reset(void);
extern bool aclcheck_track_was_checked(Oid classId, Oid objectId);

/*
 * Only record aclchecks that are dependency-relevant:
 * - ACL_CREATE on any object (creating something in a container)
 * - ACL_USAGE on non-namespace objects (using a type, language, server)
 * - ACL_EXECUTE on functions
 * - ACL_TRIGGER, ACL_REFERENCES on relations
 *
 * Skip ACL_USAGE on namespaces — that's name resolution, not dependency.
 */
static inline void
aclcheck_track_record(Oid classId, Oid objectId, AclMode mode)
{
	/* Skip ACL_USAGE on namespaces (name resolution, not dependency) */
	if (classId == NamespaceRelationId && !(mode & ACL_CREATE))
		return;

	/* Only track dependency-relevant modes */
	if (!(mode & (ACL_CREATE | ACL_USAGE | ACL_EXECUTE | ACL_TRIGGER | ACL_REFERENCES)))
		return;

	if (aclcheck_tracked_count < ACLCHECK_TRACK_MAX)
	{
		aclcheck_tracked[aclcheck_tracked_count].classId = classId;
		aclcheck_tracked[aclcheck_tracked_count].objectId = objectId;
		aclcheck_tracked_count++;
	}
}

#else							/* !USE_ASSERT_CHECKING */

#define aclcheck_track_reset()			((void) 0)
#define aclcheck_track_record(c, o, m)	((void) 0)
#define aclcheck_track_was_checked(c, o) (false)

#endif							/* USE_ASSERT_CHECKING */

#endif							/* ACLCHECK_TRACK_H */
