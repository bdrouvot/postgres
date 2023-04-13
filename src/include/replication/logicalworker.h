/*-------------------------------------------------------------------------
 *
 * logicalworker.h
 *	  Exports for logical replication workers.
 *
 * Portions Copyright (c) 2016-2023, PostgreSQL Global Development Group
 *
 * src/include/replication/logicalworker.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef LOGICALWORKER_H
#define LOGICALWORKER_H

#include <signal.h>

#include "utils/guc.h"

extern char *synchronize_slot_names;
extern char *standby_slot_names;

extern PGDLLIMPORT volatile sig_atomic_t ParallelApplyMessagePending;

extern void ApplyWorkerMain(Datum main_arg);
extern void ParallelApplyWorkerMain(Datum main_arg);
extern void ReplSlotSyncMain(Datum main_arg);

extern bool IsLogicalWorker(void);
extern bool IsLogicalParallelApplyWorker(void);

extern void HandleParallelApplyMessageInterrupt(void);
extern void HandleParallelApplyMessages(void);

extern void LogicalRepWorkersWakeupAtCommit(Oid subid);

extern void AtEOXact_LogicalRepWorkers(bool isCommit);

extern bool check_synchronize_slot_names(char **newval, void **extra, GucSource source);
extern bool check_standby_slot_names(char **newval, void **extra, GucSource source);

#endif							/* LOGICALWORKER_H */
