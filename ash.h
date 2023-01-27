#ifndef ASH_H
#define ASH_H

#include "pandos_types.h"
#include "pandos_const.h"
#include "list.h"

static semd_t semd_table[MAXPROC];
struct list_head semdFree_h = LIST_HEAD_INIT(semdFree_h);

DEFINE_HASHTABLE(semd_h,5);
hash_init(semd_h);
unsigned bkt;

/* add PCB pointed by p in the SEMD blocked process with semAdd as key */
int insertBlocked(int *semAdd, pcb_t *p){
    hash_for_each(semd_h,bkt,ite,semd_t){
        if(&(ite->s_key) == &semAdd){
            list_add_tail(p,ite->s_procq);
            return 1;
        }
    }
    if(list_empty(&semdFree_h))
        return 0;
    else{
        hash_add(semd_h,&list_first_entry(semdFree_h, struct semd_t,),) //da rifare
    }

}


#endif
