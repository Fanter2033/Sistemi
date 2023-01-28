#ifndef ASH_H
#define ASH_H

#include "pandos_types.h"
#include "pandos_const.h"
#include "hashtable.h"

static semd_t semd_table[MAXPROC];
struct hlist_head semdFree_h = HLIST_HEAD_INIT;


DEFINE_HASHTABLE(semd_h,5);
//hash_init(semd_h);
//unsigned bkt;

/* add PCB pointed by p in the SEMD blocked process with semAdd as key */
int insertBlocked(int *semAdd, pcb_t *p){

    struct semd_t* iterator; 
    //probabilmente si ferma qui perchè non riesce ad iterare. 
    hash_for_each_possible(semd_h,iterator,s_link,*semAdd){
        list_add_tail(p,&(iterator->s_procq));
        return 0;
    }
    //da qui in poi funziona
    if (!hlist_empty(&semdFree_h)) {
        semd_PTR nodeToAdd=semdFree_h.first;
        nodeToAdd->s_key=semAdd;
        INIT_LIST_HEAD(&(nodeToAdd->s_procq));
        list_add_tail(p,&(nodeToAdd->s_procq));
        hash_add(semd_h,nodeToAdd,*semAdd);
        hlist_del(semdFree_h.first);
        return 0;
    }
    return 1;
}

void initASH(){
    for (int i=0;i<MAXPROC;i++){
        hlist_add_head(&(semd_table[i]),&semdFree_h);
    }
}


#endif
