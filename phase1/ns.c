#include "ns.h" 

 void initNamespaces(){
    static struct nsd_t type_nsd[NS_TYPE_MAX][MAXPROC];    

    for (int i=0;i<NS_TYPE_MAX;i++){
        /* initialize activeList and freeList*/
        INIT_LIST_HEAD(&type_nsFree_h[i]);
        INIT_LIST_HEAD(&type_nsList_h[i]);

        for (int j=0;j<MAXPROC;j++){
            /* add all namespaces to freeList */
            type_nsd[i][j].n_type = i;  
            list_add(&(type_nsd[i][j].n_link),&type_nsFree_h[i]);
        }
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
    
    if (p==NULL || ns==NULL) return 0;      //error cases

    p->namespaces[ns->n_type]=ns;

    if (!emptyChild(p)){
        //father's namespace is assigned to firstChild's namespace
        pcb_t* firstChild = list_first_entry(&p->p_child,struct pcb_t,p_child);
        firstChild->namespaces[ns->n_type] = ns;
        
        //father's namespace is assigned to every other child's namespace
        pcb_t* iterator = NULL;
        list_for_each_entry(iterator,&firstChild->p_sib,p_sib){
            iterator->namespaces[ns->n_type]=ns;
        }
    
    }
    return 1;
}

nsd_t *allocNamespace(int type){
    if (list_empty(&type_nsFree_h[type])) return NULL;
    //Namespace is moved from freeList to active list
    nsd_t* nsdToReturn = list_first_entry(&type_nsFree_h[type], struct nsd_t, n_link);
    list_del(type_nsFree_h[type].next);
    list_add(&(nsdToReturn->n_link),&type_nsList_h[type]);
    return nsdToReturn;
}

void freeNamespace(nsd_t *ns){
    list_del(&(ns->n_link));
    list_add(&(ns->n_link),&type_nsFree_h[ns->n_type]);
}
