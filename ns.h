#ifndef NS_H
#define NS_H

#include "pandos_types.h"
#include "pandos_const.h"
#include "./ash.h"

/*NSD liberi di tipo type liberi, qui type è solo PID*/
HIDDEN struct list_head PID_nsFree_h = LIST_HEAD_INIT(PID_nsFree_h);
 
/*Lista dei namespace attivi*/
HIDDEN struct list_head PID_nsList_h = LIST_HEAD_INIT(PID_nsList_h);

void initNamespaces(){
    static struct nsd_t PID_nsd[MAXPROC];
    for (int i=0;i<MAXPROC;i++){
        PID_nsd[i].n_type=0;
        list_add(&(PID_nsd[i].n_link),&PID_nsFree_h);
    }
}

nsd_t* getNamespace(pcb_t *p, int type){
    for (int i=0; i<NS_TYPE_MAX; i++){
    	if (p->namespaces[i]->n_type==type){
    	    return p->namespaces[i];	
    	}
    }
    return NULL;
}

int addNamespace(pcb_t *p, nsd_t *ns){
    if (p==NULL){
    	return 0;
    }
    else {
    	p->namespaces[ns->n_type]=ns;
    
    	if (!emptyChild(p)){
    		pcb_t* firstChild = list_first_entry(&p->p_child,struct pcb_t,p_child);
    		firstChild->namespaces[ns->n_type] = ns;
    		pcb_t* iterator = NULL;
    		list_for_each_entry(iterator,&firstChild->p_sib,p_sib){
        		iterator->namespaces[ns->n_type]=ns;
       	 	}
    	
    	}
    }
    return 1;
}


nsd_t *allocNamespace(int type){
    if (list_empty(&PID_nsFree_h)) return NULL;
    nsd_t* nodeToReturn = list_first_entry(&PID_nsFree_h, struct nsd_t, n_link);
    list_del(PID_nsFree_h.next);
    /*inizializzare tutti gli elementi a NULL, 0 */
    nodeToReturn->n_type=0;
    list_add(&(nodeToReturn->n_link),&PID_nsList_h);
    return nodeToReturn;
}

void freeNamespace(nsd_t *ns){
    list_add(&(ns->n_link),&PID_nsFree_h);
    list_del(&(ns->n_link));
}



#endif
