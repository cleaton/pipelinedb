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
#include "utils/builtins.h"
#include "utils/rel.h"
#include "access/htup_details.h"
#include "catalog/indexing.h"

// TODO - refactor into get_oid_from_table, and put back into 
// catalog/namespace.c

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

static AlertNotifyFunc alert_func = 0;

static void
notify_alert_worker()
{
	if (!alert_func)
		return;

	alert_func();
}

void
set_notify_func(AlertNotifyFunc fn)
{
	alert_func = fn;
}

typedef void (*PluginInitFunc)(void);

static void load_plugin(const char* plugin)
{
	PluginInitFunc plugin_init;
	plugin_init = load_external_function(plugin, "_PG_init", false, NULL);
}

void check_plugin()
{
	// XXX - do this better
	
	if (alert_func)
	{
		return;
	}

	load_plugin("pipeline_push");

	if (!alert_func)
	{
		elog(ERROR, "could not load pipeline push plugin");
	}
}

void
DefineAlert(CreateAlertStmt *stmt, const char* query_str)
{
	Oid result;
	HeapTuple tup;
	bool nulls[Natts_pipeline_alert];
	Datum values[Natts_pipeline_alert];
	NameData name_data;
	Relation pipeline_alert;
	Oid namespace;
	RangeVar *alert = stmt->into->rel;
	namespace = RangeVarGetCreationNamespace(alert);

	check_plugin();

	pipeline_alert = heap_open(PipelineAlertRelationId, AccessExclusiveLock);
	namestrcpy(&name_data, alert->relname);

	values[Anum_pipeline_alert_name - 1] = NameGetDatum(&name_data);
	values[Anum_pipeline_alert_namespace - 1] = ObjectIdGetDatum(namespace);
	values[Anum_pipeline_alert_query - 1] = CStringGetTextDatum(query_str);

	MemSet(nulls, 0, sizeof(nulls));
	tup = heap_form_tuple(pipeline_alert->rd_att, values, nulls);

	result = simple_heap_insert(pipeline_alert, tup);
	CatalogUpdateIndexes(pipeline_alert, tup);
	CommandCounterIncrement();

	notify_alert_worker(); // extension will increment a shmem ctr on xact fin
	heap_close(pipeline_alert, NoLock); // unlock after transaction
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

	notify_alert_worker(); // extension will increment a shmem ctr on xact fin
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
