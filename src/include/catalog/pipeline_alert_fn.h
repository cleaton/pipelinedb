#ifndef PIPELINE_ALERT_FN_H
#define PIPELINE_ALERT_FN_H

#include "postgres.h"

#include "catalog/pipeline_alert.h"
#include "access/htup.h"
#include "nodes/primnodes.h"

extern Oid get_alert_oid(List *name, bool missing_ok);
void RemoveAlertById(Oid oid);

HeapTuple GetPipelineAlertTuple(RangeVar *name);
bool IsAnAlert(RangeVar *name);

#endif
