#ifndef NS_H
#define NS_H

#include "pandos_types.h"
#include "pandos_const.h"
#include "pcb.h"

/* Initialize all free namespaces lists */
void initNamespaces();

/* return namespace of type "type" associated with p */
nsd_t* getNamespace(pcb_t *p, int type);

/* Associate the namespace ns to the p process and all its children , 
   return FALSE in case of error */
int addNamespace(pcb_t *p, nsd_t *ns);

/* Alloc a namespace of type "type" from the correct free list (NULL if no more free NS)*/
nsd_t *allocNamespace(int type);

/* Free a namespace ns by inserting it in the correct namespace free list */
void freeNamespace(nsd_t *ns);


#endif
