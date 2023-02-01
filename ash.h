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
    hash_for_each_possible(semd_h,iterator,s_link,*semAdd){
        insertProcQ(&(iterator->s_procq),p);
        return 0;
    }

    if (!list_empty(&semdFree_h)) {
        semd_t* nodeToAdd = list_first_entry(&semdFree_h,s_freelink);
    
        /* node initialization */
        nodeToAdd->s_key = semAdd;
        mkEmptyProcQ(&(nodeToAdd->s_procq));  
        insertProcQ(&(nodeToAdd->s_procq),p);

        hash_add(semd_h,&(nodeToAdd->s_link), *(nodeToAdd->s_key));
        list_del(&(nodeToAdd->s_freelink));
        return 0;
    }
    return 1;
}

/* Pietro aggiungi il commento :) */

pcb_t* removeBlocked(int *semAdd){
    pcb_t* pcbToReturn = NULL;
    semd_t* iterator;
    hash_for_each_possible(semd_h,iterator,s_link,*semAdd){
        pcbToReturn = removeProcQ(&iterator->s_procq);        
        if (emptyProcQ(&iterator->s_procq)){
            list_add(&(iterator->s_freelink),&semdFree_h);
            hash_del(&(iterator->s_link));
        }
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