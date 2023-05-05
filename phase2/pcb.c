#include "pcb.h"

/* pcbFree_h - pcb that are free or not used */
HIDDEN LIST_HEAD(pcbFree_h);

void initPcbs(){
    static pcb_t pcbFree_table[MAXPROC];
    for (int i=0;i<MAXPROC;i++)
        list_add(&(pcbFree_table[i].p_list),&pcbFree_h);
}

void freePcb(pcb_t* p){
    list_add(&(p->p_list),&pcbFree_h);
}

pcb_t* allocPcb(){
    if (list_empty(&pcbFree_h)) return NULL;  //case1: there is no more PCB available

    /*case2: take first PCB from pcbFree_h*/

    pcb_t* pcbToReturn = list_first_entry(&pcbFree_h, struct pcb_t, p_list);      
    list_del(&pcbToReturn->p_list);

    /* initializing parameters */
    pcbToReturn->p_parent=NULL;              
    INIT_LIST_HEAD(&pcbToReturn->p_child);
    INIT_LIST_HEAD(&pcbToReturn->p_sib);
    pcbToReturn->p_semAdd=NULL;
    for (int i=0;i<NS_TYPE_MAX;i++){
        pcbToReturn->namespaces[i]=NULL;      
    }

    return pcbToReturn;
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
    if (emptyProcQ(head)) return NULL;  //case1: no pcb to remove
    /* case2: del first pcb from the list */

    pcb_t* pcbToReturn = list_first_entry(head, struct pcb_t, p_list);           
    list_del(&pcbToReturn->p_list);
    return pcbToReturn;
}

pcb_t* outProcQ(struct list_head* head, pcb_t* p){
    pcb_t* iterator = NULL;
    list_for_each_entry(iterator,head,p_list){
        if (iterator == p){      // p is in the list
            list_del(&iterator->p_list);
            return iterator;
        }
    }
    return NULL;
}    

int emptyChild(pcb_t *p){
    return list_empty(&p->p_child);
}

void insertChild(pcb_t* prnt, pcb_t* p){
    if (emptyChild(prnt)) prnt->p_child.next = &p->p_child; //case1: no child, p in the first
    else {          //case2: p will be in the sib list of the child
        pcb_t* firstChild = list_first_entry(&prnt->p_child,struct pcb_t,p_child); 
        list_add_tail(&p->p_sib,&firstChild->p_sib);
    }
    p->p_parent=prnt;
}

pcb_t* removeChild(pcb_t* p){
    if (emptyChild(p)) return NULL;   //case1: p has no child
    
    pcb_t* firstChild = list_first_entry(&p->p_child,struct pcb_t, p_child);
    if (list_empty(&firstChild->p_sib)) INIT_LIST_HEAD (&p->p_child);   //case2: firstChild is only child
    else {   //case3: more children
        pcb_t* secondChild = list_first_entry(&firstChild->p_sib,struct pcb_t,p_sib); //secondChild = firstSib
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
    if (p == firstChild) removeChild(prnt); //case1: p is the first child, using removeChild 
    else {                                                                        
        p->p_parent = NULL;
        list_del_init(&p->p_sib);
    }
    return p;
}


pcb_t* findPCBfromQUEUE(int pid, struct list_head* head ){
    pcb_t* iterator = NULL;
    list_for_each_entry(iterator,head,p_list){
        if (iterator->p_pid == pid)      // p is in the list
            return iterator;
    }
    return NULL;
}
