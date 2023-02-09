#ifndef PCB_H
#define PCB_H

#include "pandos_types.h"
#include "pandos_const.h"
#include "list.h"

/* pcbFree_h - pcb that are free or not used */
HIDDEN struct list_head pcbFree_h = LIST_HEAD_INIT(pcbFree_h);

/* init the pcbFree list */
void initPcbs(){
    static pcb_t pcbFree_table[MAXPROC];
    for (int i=0;i<MAXPROC;i++){
        list_add(&(pcbFree_table[i].p_list),&pcbFree_h);
    }
}

/* add the element pointed by p in pcbFree_h */
void freePcb(pcb_t* p){
    list_add(&(p->p_list),&pcbFree_h);
}

/* if pcbFree is empty then NULL else remove an element from pcbFree_h */
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

/* -- Queue list management -- */

/* create a empty pcb list */
void mkEmptyProcQ(struct list_head *head){
    INIT_LIST_HEAD(head);
}

/* if PCB list is empty then TRUE, FALSE otherwise  */
int emptyProcQ(struct list_head *head){
    return list_empty(head);
}

/* insert p's pointer into PCB queue */
void insertProcQ(struct list_head* head, pcb_t* p){
    list_add_tail(&p->p_list,head);
}

/* return the head element from PCB queue, NULL otherwise */
pcb_t* headProcQ(struct list_head* head){
    if (list_empty(head)) return NULL;
    return list_first_entry(head, struct pcb_t,p_list);
}

/* removes and return the first element from PCB queue, return NULL if the queue is empty*/
pcb_t* removeProcQ(struct list_head* head){
    if (list_empty(head)) return NULL;
    pcb_t* nodeToReturn = list_first_entry(head, struct pcb_t, p_list);
    list_del(&nodeToReturn->p_list);
    return nodeToReturn;
}

/* removes the PCB pointed by p from the PCB queue, if P isn't in the queue then return NULL*/
pcb_t* outProcQ(struct list_head* head, pcb_t* p){
    
    pcb_t* iterator = NULL;
    pcb_t* tmpStorage = NULL;
    pcb_t* pcbToReturn = NULL;

    list_for_each_entry(iterator,head,p_list){
        if (iterator == p){
            list_del(&iterator->p_list);
            return iterator;
        }
    }
    return NULL;
}


/* tree management */

int emptyChild(pcb_t *p){
    return list_empty(&(p->p_child));
}

void insertChild(pcb_t *prnt, pcb_t *p){
    if (emptyChild(prnt)){
        p->p_parent = prnt;
        list_add(p,&(prnt->p_child));
    }
    else{
        pcb_t* first_child = list_first_entry(&prnt->p_child,struct pcb_t, p_child);
        p->p_parent = prnt;
        list_add(&p->p_sib,&(first_child->p_sib));
    }
}

pcb_t* removeChild(pcb_t *p){
    if (emptyChild(p)) return NULL; //caso: padre non ha figli
    pcb_t* first_child = list_first_entry(&p->p_child, struct pcb_t, p_child);
 
    if (list_empty(&first_child->p_sib)) {
        INIT_LIST_HEAD(&p->p_child); //caso: un solo figlio
        addokbuf("\n rimuovo figlio unico");
        }
    else{ 
        p->p_child=first_child->p_sib; //caso: figlio con fratelli
        list_del(&first_child->p_sib);
        addokbuf("\n rimuovo figlio con fratelli");
    }
    first_child->p_parent=NULL;
    return first_child;
}

pcb_t* outChild(pcb_t* p){  
    addokbuf("inizio outchild \n");
    if (p->p_parent == NULL) return NULL;
    pcb_t* iterator = NULL;
    pcb_t* bucket = NULL;

    
    list_for_each_entry_safe(iterator,bucket,&(p->p_parent->p_child), p_sib){
        addokbuf("sono nel ciclo \n");
        if (iterator == p){
            addokbuf("sono nel ciclo e nella condizione\n");
            if (iterator==list_first_entry(&(p->p_parent->p_child),struct pcb_t,p_child)){
                 addokbuf("sono nel ciclo e nella seconda condizione\n");
                return removeChild(iterator->p_parent);

            }
                
            list_del(&iterator->p_sib);
            iterator->p_parent=NULL;
            return iterator;
        }
        addokbuf("condizione non verificata \n");
    }
    addokbuf(" finisco il ciclo \n ");
    return p;
}

#endif //PCB_H