#ifndef ASH_H
#define ASH_H

#include "pandos_types.h"
#include "pandos_const.h"
#include "hashtable.h"
#include "pcb.h"

static semd_t semd_table[MAXPROC];
struct list_head semdFree_h = LIST_HEAD_INIT(semdFree_h);


DECLARE_HASHTABLE(semd_h,5);

/* add PCB pointed by p in the SEMD blocked process with semAdd as key */
int insertBlocked(int *semAdd, pcb_t *p){

    struct semd_t* iterator; 
    hash_for_each_possible(semd_h,iterator,s_link,*semAdd){
        list_add_tail(p,&(iterator->s_procq));
        addokbuf("aggiungo pcb a semaforo esistente   \n");
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

        addokbuf("aggiungo semaforo a ash\n");
        return 0;
    }

    addokbuf("nulla da fare \n");
    return 1;
}

void initASH(){
    for (int i=0;i<MAXPROC;i++){
        list_add(&(semd_table[i]),&semdFree_h);
    }
}


#endif