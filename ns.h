#ifndef NAMESPACES_H
#define NAMESPACES_H

#include "pandos_types.h"
#include "pandos_const.h"
#include "./ash.h"

/*NSD liberi di tipo type liberi, qui type è solo PID*/
HIDDEN struct list_head PID_nsFree_h = LIST_HEAD_INIT(PID_nsFree_h);
 
/*Lista dei namespace attivi*/
HIDDEN struct list_head PID_nsList_h = LIST_HEAD_INIT(PID_nsList_h);

void initNamespaces(){
    static nsd_t PID_nsd[MAXPROC];
    for (int i=0;i<MAXPROC;i++){
        PID_nsd[i].n_type=0;
        list_add(&(PID_nsd[i].n_link),&PID_nsFree_h);
    }
}

nsd_t* getNamespace(pcb_t *p, int type){
    nsd_t* iterator = NULL;
    list_for_each_entry(iterator,p->namespaces,n_link){
        if (iterator->n_type == type){
            return iterator;
        }
    }
    return NULL;
}

int addNamespace(pcb_t *p, nsd_t *ns){
    list_add(&ns->n_link,&p->namespaces[ns->n_type]);
    
    if (!emptyChild(p)){
    	pcb_t* firstChild = list_first_entry(&p->p_child,struct pcb_t,p_child); 
    	list_add(&ns->n_link,&firstChild->namespaces[ns->n_type]);
    	pcb_t* iterator = NULL;
    	list_for_each_entry(iterator,&firstChild->p_sib,p_sib){
        	list_add(&ns->n_link,&iterator->namespaces[ns->n_type]);
        }
    	
    }
   
    return 1;
}

/*Attualmente abbiamo solo un tipo di namespace, quando ne
serviranno di più dovremo avere un array di liste tra cui scegliere */

nsd_t *allocNamespace(int type){
    if (list_empty(&PID_nsFree_h)) return NULL;
    nsd_t* nodeToReturn = list_first_entry(&PID_nsFree_h, struct nsd_t, n_link);
    list_del(PID_nsFree_h.next);
    /*inizializzare tutti gli elementi a NULL, 0 */
    nodeToReturn->n_type=0;
    return nodeToReturn;
}

void freeNamespace(nsd_t *ns){
    list_add(&(ns->n_link),&PID_nsFree_h);
}



#endif
