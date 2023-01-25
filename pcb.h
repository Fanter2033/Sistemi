#ifndef PCB_H
#define PCB_H

#include "pandos_types.h"
#include "pandos_const.h"
#include "list.h"

/* init, alloc and free */

HIDDEN struct list_head pcbFree_h = LIST_HEAD_INIT(pcbFree_h);

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
    pcb_t* nodeToReturn = pcbFree_h.next;
    list_del(pcbFree_h.next);
    /*inizializzare tutti gli elementi a NULL, 0 */
    return nodeToReturn;
}

/* Queue list management */
void mkEmptyProcQ(struct list_head *head){
    INIT_LIST_HEAD(head);
}

int emptyProcQ(struct list_head *head){
    return list_empty(head);
}

void insertProcQ(struct list_head* head, pcb_t* p){
    list_add_tail(p,head);
}

pcb_t* headProcQ(struct list_head* head){
    if (list_empty(head)) return NULL;
    return list_first_entry(head, struct pcb_t,p_list);
}

pcb_t* removeProcQ(struct list_head* head){
    if (list_empty(head)) return NULL;
    pcb_t* nodeToReturn = list_first_entry(head, struct pcb_t, p_list);
    list_del(nodeToReturn);
    return nodeToReturn;
}

pcb_t* outProcQ(struct list_head* head, pcb_t* p){
    struct pcb_t* iterator=NULL;
    list_for_each_entry(iterator, head, p_list){
        if ( iterator == p ){
            list_del(iterator);
            return iterator;
        }
    }
    return NULL;
}

#endif //PCB_H