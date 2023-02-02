#ifndef ASH_H
#define ASH_H

#include "pandos_types.h"
#include "pandos_const.h"
#include "hashtable.h"
#include "pcb.h"


HIDDEN struct list_head semdFree_h = LIST_HEAD_INIT(semdFree_h);



DECLARE_HASHTABLE(semd_h,5);

/* add PCB pointed by p in the SEMD blocked process with semAdd as key */
int insertBlocked(int *semAdd, pcb_t *p){
    struct semd_t* iterator; 
    hash_for_each_possible(semd_h,iterator,s_link, semAdd){
        p->p_semAdd = semAdd;
        insertProcQ(&(iterator->s_procq),p);
        return 0;
    }

    if (!list_empty(&semdFree_h)) {
        semd_t* nodeToAdd = list_first_entry(&semdFree_h,semd_t,s_freelink);
    
        /* node initialization */
        nodeToAdd->s_key = semAdd;
        mkEmptyProcQ(&(nodeToAdd->s_procq)); 
        p->p_semAdd = semAdd; 
        insertProcQ(&(nodeToAdd->s_procq),p);

        hash_add(semd_h,&(nodeToAdd->s_link), (nodeToAdd->s_key));
        list_del(&(nodeToAdd->s_freelink));
        return 0;
    }
    return 1;
}

/* return the first pcb from the queue of proc. of SEMD with semAdd as key, 
   remove the relative descriptor from the ASH (and relink it in the semdFree_h) */
pcb_t* removeBlocked(int *semAdd){
    pcb_t* pcbToReturn = NULL;
    semd_t* iterator;
    struct hlist_node* tmp = NULL;
    hash_for_each_possible_safe(semd_h,iterator,tmp,s_link,semAdd){ 
        pcbToReturn = removeProcQ(&iterator->s_procq);    
        pcbToReturn->p_semAdd = NULL;   
        if (emptyProcQ(&iterator->s_procq)){
            list_add(&(iterator->s_freelink),&semdFree_h);
            hash_del(&(iterator->s_link));
        }
    }
    return pcbToReturn;
}

/* remove the PCB pointed by p (and eventually the semaphore descriptor when the queue is empty) from the queue which is blocked (p->semAdd), 
if (there is no pcb ) return NULL else return p*/
pcb_t* outBlocked(pcb_t* p){
    semd_t* iterator;
    struct hlist_node* tmp=NULL;

    pcb_t* pcbToReturn = NULL;
    hash_for_each_possible_safe(semd_h,iterator,tmp,s_link,(p->p_semAdd)){
        pcbToReturn = outProcQ(&iterator->s_procq,p);

        if (emptyProcQ(&iterator->s_procq)){
            list_add(&(iterator->s_freelink),&semdFree_h);
            hash_del(&(iterator->s_link));
        }
    }
    return pcbToReturn;
}

/* return (without remove) the PCB pointer located in the head of the SEMD proc. queue 
 if (there isn't the SEMD in ASH || its proc queue is empty) */
pcb_t* headBlocked(int *semAdd){
    pcb_t* pcbToReturn = NULL;
    semd_t* iterator;
    hash_for_each_possible(semd_h,iterator,s_link,semAdd){
        pcbToReturn=headProcQ(&iterator->s_procq);
    }
    return pcbToReturn;
}

void initASH(){
    static semd_t semd_table[MAXPROC];
    for (int i=0;i<MAXPROC;i++){
        list_add(&(semd_table[i].s_freelink),&semdFree_h);
    }
}


#endif