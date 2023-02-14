#include "pcb.h"

void initPcbs(){
    static pcb_t pcbFree_table[MAXPROC];
    for (int i=0;i<MAXPROC;i++){
        list_add(&(pcbFree_table[i].p_list),&pcbFree_h);
    }
}

void freePcb(pcb_t* p){
    list_add(&(p->p_list),&pcbFree_h);
}

pcb_t* allocPcb(){
    if (list_empty(&pcbFree_h)) return NULL;
    pcb_t* nodeToReturn = list_first_entry(&pcbFree_h, struct pcb_t, p_list);
    list_del(pcbFree_h.next);
    /*inizializzare tutti gli elementi a NULL, 0 */
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
    if (emptyProcQ(head)) return NULL;
    pcb_t* nodeToReturn = list_first_entry(head, struct pcb_t, p_list);
    list_del(&nodeToReturn->p_list);
    return nodeToReturn;
}

pcb_t* outProcQ(struct list_head* head, pcb_t* p){
    pcb_t* iterator = NULL;
    list_for_each_entry(iterator,head,p_list){
        if (iterator == p){
            list_del(&iterator->p_list);
            return iterator;
        }
    }
    return NULL;
}

int emptyChild(pcb_t *p){
    return p->p_child.next==&p->p_child;
}

void insertChild(pcb_t* prnt, pcb_t* p){

    if (emptyChild(prnt))   //caso: non ci sono figli
        prnt->p_child.next= &p->p_child;
    else {      
        pcb_t* firstChild = list_first_entry(&prnt->p_child,struct pcb_t,p_child);
        list_add_tail(&p->p_sib,&firstChild->p_sib);
        /*
        inserimento con add : 1->9->8->7...
        inserimento con add_tail: 1->2->3->...
        */
    }
    p->p_parent=prnt;
}

pcb_t* removeChild(pcb_t* p){

    if (emptyChild(p)) return NULL;
    pcb_t* firstChild = list_first_entry(&p->p_child,struct pcb_t, p_child);

    if (list_empty(&firstChild->p_sib))       //caso figlio unico
        INIT_LIST_HEAD (&p->p_child);
    else {                                  //caso più figli  
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
    if (p == firstChild){       //caso: p è il primo figlio
        removeChild(prnt);
    }
    else {                  //caso: p è un figlio intermedio
        p->p_parent = NULL;
        list_del_init(&p->p_sib);
    }
    return p;
}