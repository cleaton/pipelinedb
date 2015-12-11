/*-------------------------------------------------------------------------
 *
 * pipeline_alert.c
 *	  routines to support manipulation of the pipeline_alert relation
 *
 * Copyright (c) 2013-2015, PipelineDB
 *
 * IDENTIFICATION
 *	  src/backend/catalog/pipeline_alert.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "nodes/pg_list.h"
#include "catalog/pipeline_alert_fn.h"
#include "catalog/namespace.h"
#include "utils/syscache.h"
#include "utils/relcache.h"
#include "access/heapam.h"
#include "access/xact.h"

// TODO - refactor into get_oid_from_table, and put back into 
// catalog/namespace.c
// ExecDropStmt

Oid
get_alert_oid(List *name, bool missing_ok)
{
	char	   *schemaname;
	char	   *alert_name;
	Oid			namespaceId;
	Oid			alertoid = InvalidOid;
	ListCell   *l;

	/* deconstruct the name list */
	DeconstructQualifiedName(name, &schemaname, &alert_name);

	if (schemaname)
	{
		/* use exact schema given */
		namespaceId = LookupExplicitNamespace(schemaname, missing_ok);
		if (missing_ok && !OidIsValid(namespaceId))
			alertoid = InvalidOid;
		else
			alertoid = GetSysCacheOid2(PIPELINEALERTNAMESPACENAME,
									 ObjectIdGetDatum(namespaceId),
									 PointerGetDatum(alert_name));
	}
	else
	{
		/* search for it in search path */
		recomputeNamespacePath();

		foreach(l, activeSearchPath)
		{
			namespaceId = lfirst_oid(l);

			if (namespaceId == myTempNamespace)
				continue;		/* do not look in temp namespace */

			alertoid = GetSysCacheOid2(PIPELINEALERTNAMESPACENAME,
									 ObjectIdGetDatum(namespaceId),
									 PointerGetDatum(alert_name));

			if (OidIsValid(alertoid))
				return alertoid;
		}
	}

	/* Not found in path */
	if (!OidIsValid(alertoid) && !missing_ok)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("alert \"%s\" does not exist",
						NameListToString(name))));

	return alertoid;
}

void
RemoveAlertById(Oid oid)
{
	// XXX - do push sequence notify.

	Relation pipeline_alert;
	HeapTuple tuple;

	pipeline_alert = heap_open(PipelineAlertRelationId, ExclusiveLock);
	tuple = SearchSysCache1(PIPELINEALERTOID, ObjectIdGetDatum(oid));

	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cache lookup failed for alert with OID %u", oid);

	simple_heap_delete(pipeline_alert, &tuple->t_self);

	ReleaseSysCache(tuple);
	CommandCounterIncrement();
	heap_close(pipeline_alert, NoLock); // drop after xact
}

HeapTuple
GetPipelineAlertTuple(RangeVar *name)
{
	HeapTuple tuple;
	Oid namespace;

	if (name->schemaname == NULL)
		namespace = RangeVarGetCreationNamespace(name);
	else
		namespace = get_namespace_oid(name->schemaname, false);

	Assert(OidIsValid(namespace));

	tuple = SearchSysCache2(PIPELINEALERTNAMESPACENAME, ObjectIdGetDatum(namespace), CStringGetDatum(name->relname));

	return tuple;
}

bool
IsAnAlert(RangeVar *name)
{
	HeapTuple tuple = GetPipelineAlertTuple(name);
	if (!HeapTupleIsValid(tuple))
		return false;
	ReleaseSysCache(tuple);
	return true;
}
