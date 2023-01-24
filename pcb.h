#ifndef PCB_H
#define PCB_H

#include "./pandos_types.h"
#include "./pandos_const.h"
#include "./list.h"

pcb_t pcbFree_table[MAXPROC];

struct list_head pcbFree_h = LIST_HEAD_INIT(pcbFree_h);

//1.
void initPcbs(){
    //inizializzazione p_list di ogni pcb (?)

    for (int i=0;i<MAXPROC;i++){
        list_add(&(pcbFree_table[i].p_list),&pcbFree_h);
    }
}

//2.
void freePcb(pcb_t* p){
    list_add(&(p->p_list),&pcbFree_h);
}



#endif 
