#ifndef NS_H
#define NS_H

#include "pandos_types.h"
#include "pandos_const.h"
#include "./ash.h"

/*NSD liberi di tipo type liberi, qui type è solo PID*/
HIDDEN struct list_head PID_nsFree_h = LIST_HEAD_INIT(PID_nsFree_h);
 
/*Lista dei namespace attivi*/
HIDDEN struct list_head PID_nsList_h = LIST_HEAD_INIT(PID_nsList_h);

void initNamespaces();

nsd_t* getNamespace(pcb_t *p, int type);

int addNamespace(pcb_t *p, nsd_t *ns);

nsd_t *allocNamespace(int type);

void freeNamespace(nsd_t *ns);


#endif
