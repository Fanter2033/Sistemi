#ifndef NS_H
#define NS_H

#include "pandos_types.h"
#include "pandos_const.h"
#include "pcb.h"

/* list of active namespace of type "type", in this case the only type is "PID" */
HIDDEN struct list_head type_nsFree_h[NS_TYPE_MAX];
 
/* Active namespaces list*/
HIDDEN struct list_head type_nsList_h[NS_TYPE_MAX];

/* Initialize all free namespaces */
void initNamespaces();

/* return namespace of type "type" associated with the p process. */
nsd_t* getNamespace(pcb_t *p, int type);

/* Associate the namespace ns to the p process and all its children , 
   if (there is an error) return TRUE else FALSE */
int addNamespace(pcb_t *p, nsd_t *ns);

/* Alloc a namespace of type "type" frmm the correct list */
nsd_t *allocNamespace(int type);

/* Free a namespace ns by inserting it in the correct namespace list */
void freeNamespace(nsd_t *ns);


#endif
