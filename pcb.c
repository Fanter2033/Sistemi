#include "pcb.h"

void initPcbs(){
    static pcb_t pcbFree_table[MAXPROC];
    for (int i=0;i<MAXPROC;i++)
        list_add(&(pcbFree_table[i].p_list),&pcbFree_h);
}

void freePcb(pcb_t* p){
    list_add(&(p->p_list),&pcbFree_h);
}

pcb_t* allocPcb(){
    if (list_empty(&pcbFree_h)) return NULL;                                       //case1: there is no more PCB available

    pcb_t* nodeToReturn = list_first_entry(&pcbFree_h, struct pcb_t, p_list);      //case2: take first PCB from pcbFree_h
    list_del(pcbFree_h.next);
    //initializing parameters  
    nodeToReturn->p_parent=NULL;              
    INIT_LIST_HEAD(&nodeToReturn->p_child);
    INIT_LIST_HEAD(&nodeToReturn->p_sib);
    nodeToReturn->p_semAdd=NULL;
    return nodeToReturn;
}

void mkEmptyProcQ(struct list_head *head){
    INIT_LIST_HEAD(head);
}

int emptyProcQ(struct list_head *head){
    return list_empty(head);
}

void insertProcQ(struct list_head* head, pcb_t* p){
    list_add_tail(&p->p_list,head);
}

pcb_t* headProcQ(struct list_head* head){
    if (emptyProcQ(head)) return NULL;                 
    return list_first_entry(head, struct pcb_t,p_list);
}

pcb_t* removeProcQ(struct list_head* head){
    if (emptyProcQ(head)) return NULL;                                             //case1: no pcb to reomve

    pcb_t* nodeToReturn = list_first_entry(head, struct pcb_t, p_list);            //case2: del first pcb from the list
    list_del(&nodeToReturn->p_list);
    return nodeToReturn;
}

pcb_t* outProcQ(struct list_head* head, pcb_t* p){
    pcb_t* iterator = NULL;
    int p_inlist = 0;
    list_for_each_entry(iterator,head,p_list){
        if (iterator == p){               // p is in the list
            list_del(&iterator->p_list);
            p_inlist = 1; break;
        }
    }
    if(p_inlist) return iterator;
    else         return NULL;
}

int emptyChild(pcb_t *p){
    return p->p_child.next == &p->p_child;
}

void insertChild(pcb_t* prnt, pcb_t* p){
    if (emptyChild(prnt))                                                          //case1: no child, p in the first
        prnt->p_child.next = &p->p_child;
    else {      
        pcb_t* firstChild = list_first_entry(&prnt->p_child,struct pcb_t,p_child); //case2: p will be in the sib list of the child
        list_add_tail(&p->p_sib,&firstChild->p_sib);
    }
    p->p_parent=prnt;
}

pcb_t* removeChild(pcb_t* p){
    if (emptyChild(p)) return NULL;                                                //case1:  no child
    
    pcb_t* firstChild = list_first_entry(&p->p_child,struct pcb_t, p_child);
    if (list_empty(&firstChild->p_sib))                                            //case2: p is only child
        INIT_LIST_HEAD (&p->p_child);
    else {                                                                         //case2: more children, p' sib become the child of the father
        pcb_t* secondChild = list_first_entry(&firstChild->p_sib,struct pcb_t,p_sib);
        p->p_child.next = &secondChild->p_child;
        list_del_init(&firstChild->p_sib); 
    }
    firstChild->p_parent=NULL;

    return firstChild;
}


pcb_t* outChild(pcb_t* p){
    if (p->p_parent == NULL) return NULL;

    pcb_t* prnt= p->p_parent;
    pcb_t* firstChild = list_first_entry(&prnt->p_child,struct pcb_t, p_child);
    if (p == firstChild){                                                          //case1: p is the first child, using removeChild 
        removeChild(prnt);
    }
    else {                                                                         //caso2: otherwise
        p->p_parent = NULL;
        list_del_init(&p->p_sib);
    }
    return p;
}