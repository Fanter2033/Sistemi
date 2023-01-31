#ifndef ASH_H
#define ASH_H

#include "pandos_types.h"
#include "pandos_const.h"
#include "hashtable.h"
#include "pcb.h"

static semd_t semd_table[MAXPROC];
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
        semd_t* nodeToAdd = semdFree_h.next;
        /* node initialization */
        *(nodeToAdd->s_key) = *semAdd;
        mkEmptyProcQ(&(nodeToAdd->s_procq));    
        insertProcQ(&(nodeToAdd->s_procq),p);
        
        hash_add(semd_h,&(nodeToAdd->s_link), *(nodeToAdd->s_key));
        list_del(semdFree_h.next);

        return 0;
    }
    return 1;
}

void initASH(){
    for (int i=0;i<MAXPROC;i++){
        //non siamo sicuri, così s_freelink non viene utilizzato *per ora*
        list_add(&(semd_table[i]),&semdFree_h);
    }
}


#endif