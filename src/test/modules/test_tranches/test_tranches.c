#include "postgres.h"

#include "fmgr.h"
#include "miscadmin.h"
#include "storage/dsm_registry.h"
#include "storage/ipc.h"
#include "storage/lwlock.h"
#include "storage/shmem.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/injection_point.h"
#include "utils/wait_classes.h"

PG_MODULE_MAGIC;

/* hooks */
static shmem_request_hook_type prev_shmem_request_hook = NULL;
static shmem_startup_hook_type prev_shmem_startup_hook = NULL;
static void test_tranches_shmem_request(void);
static void test_tranches_shmem_startup(void);

/* GUC */
static int	test_tranches_requested_named_tranches = 0;

typedef struct testTranchesSharedState
{
	int			next_index;
}			testTranchesSharedState;

static testTranchesSharedState * test_lwlock_ss = NULL;

/*
 * LWLock wait event masks. Copied from src/backend/utils/activity/wait_event.c
 */
#define WAIT_EVENT_CLASS_MASK	0xFF000000

/*
 * Module load callback
 */
void
_PG_init(void)
{
	prev_shmem_request_hook = shmem_request_hook;
	shmem_request_hook = test_tranches_shmem_request;
	prev_shmem_startup_hook = shmem_startup_hook;
	shmem_startup_hook = test_tranches_shmem_startup;

	DefineCustomIntVariable("test_tranches.requested_named_tranches",
							"Sets the number of locks created during shmem request",
							NULL,
							&test_tranches_requested_named_tranches,
							2,
							0,
							UINT16_MAX,
							PGC_POSTMASTER,
							0,
							NULL,
							NULL,
							NULL);
}

static void
test_tranches_shmem_startup(void)
{
	bool		found;

	if (prev_shmem_startup_hook)
		prev_shmem_startup_hook();

	test_lwlock_ss = NULL;

	test_lwlock_ss = ShmemInitStruct("test_tranches",
									 sizeof(testTranchesSharedState),
									 &found);
	if (!found)
	{
		test_lwlock_ss->next_index = test_tranches_requested_named_tranches;
	}
}

static Size
test_tranches_memsize(void)
{
	Size		size;

	size = MAXALIGN(sizeof(testTranchesSharedState));

	return size;
}

static void
test_tranches_shmem_request(void)
{
	int			i = 0;

	if (prev_shmem_request_hook)
		prev_shmem_request_hook();

	RequestAddinShmemSpace(test_tranches_memsize());

	for (i = 0; i < test_tranches_requested_named_tranches; i++)
	{
		char		name[15];

		snprintf(name, sizeof(name), "test_lock_%d", i);
		RequestNamedLWLockTranche(name, i);
	}
}

PG_FUNCTION_INFO_V1(test_tranches_new);
Datum
test_tranches_new(PG_FUNCTION_ARGS)
{
	int64		num = PG_GETARG_INT64(0);
	int			i;

	for (i = test_lwlock_ss->next_index; i < num + test_lwlock_ss->next_index; i++)
	{
		char		name[50];

		snprintf(name, 50, "test_lock__%d", i);

		LWLockNewTrancheId(name);
	}

	test_lwlock_ss->next_index = i;

	PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(test_tranches_new_tranche);
Datum
test_tranches_new_tranche(PG_FUNCTION_ARGS)
{
	char	   *tranche_name = NULL;

	if (!PG_ARGISNULL(0))
		tranche_name = text_to_cstring(PG_GETARG_TEXT_PP(0));

	PG_RETURN_INT32(LWLockNewTrancheId(tranche_name));
}

PG_FUNCTION_INFO_V1(test_tranches_lookup);
Datum
test_tranches_lookup(PG_FUNCTION_ARGS)
{
	const char *tranche_name = GetLWLockIdentifier(PG_WAIT_LWLOCK & WAIT_EVENT_CLASS_MASK, PG_GETARG_INT32(0));

	if (tranche_name)
		PG_RETURN_TEXT_P(cstring_to_text(tranche_name));
	else
		PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(test_tranches_get_named_lwlock);
Datum
test_tranches_get_named_lwlock(PG_FUNCTION_ARGS)
{
	int			i;
	LWLockPadded *locks;

	locks = GetNamedLWLockTranche(text_to_cstring(PG_GETARG_TEXT_PP(0)));

	for (i = 0; i < PG_GETARG_INT32(1); i++)
	{
		LWLock	   *lock = &locks[i].lock;

		LWLockAcquire(lock, LW_SHARED);
		LWLockRelease(lock);
	}

	PG_RETURN_INT32(i);
}

PG_FUNCTION_INFO_V1(test_tranches_get_first_user_defined);
Datum
test_tranches_get_first_user_defined(PG_FUNCTION_ARGS)
{
	return LWTRANCHE_FIRST_USER_DEFINED;
}

PG_FUNCTION_INFO_V1(test_tranches_lwlock_initialize);
Datum
test_tranches_lwlock_initialize(PG_FUNCTION_ARGS)
{
	LWLock		lock;

	LWLockInitialize(&lock, PG_GETARG_INT32(0));

	PG_RETURN_VOID();
}
