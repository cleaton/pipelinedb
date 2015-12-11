/*-------------------------------------------------------------------------
 *
 * pipeline_alert.h
 *	  definition of continuous queries that have been registered
 *
 * Copyright (c) 2013-2015, PipelineDB
 *
 * src/include/catalog/pipeline_alert.h
 *
 * NOTES
 *	  the genbki.pl script reads this file and generates .bki
 *	  information from the DATA() statements.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PIPELINE_ALERT_H
#define PIPELINE_ALERT_H

#include "catalog/genbki.h"
#include "datatype/timestamp.h"

/* ----------------------------------------------------------------
 *		pipeline_alert definition.
 *
 *		cpp turns this into typedef struct FormData_pipeline_alert
 * ----------------------------------------------------------------
 */
#define PipelineAlertRelationId  4257

CATALOG(pipeline_alert,4257)
{
	Oid			namespace;
	NameData	name;
#ifdef CATALOG_VARLEN
	text 		query;
#endif

} FormData_pipeline_alert;

/* ----------------
 *		Form_pipeline_alert corresponds to a pointer to a tuple with
 *		the format of the pipeline_alert relation.
 * ----------------
 */
typedef FormData_pipeline_alert *Form_pipeline_alert;

/* ----------------
 *		compiler constants for pipeline_alert
 * ----------------
 */
#define Natts_pipeline_alert			3
#define Anum_pipeline_alert_namespace	1
#define Anum_pipeline_alert_name		2
#define Anum_pipeline_alert_query 		3

#endif
