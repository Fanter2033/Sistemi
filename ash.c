#include "ash.h"

static void removeEmptySemd(semd_t* s){
    if (emptyProcQ(&s->s_procq)){
        hash_del(&(s->s_link));
        list_add(&(s->s_freelink),&semdFree_h);
    }
}

int insertBlocked(int *semAdd, pcb_t *p){
    if (p->p_semAdd!=NULL) return 1;
    struct semd_t* iterator; 
    hash_for_each_possible(semd_h,iterator,s_link, semAdd){
        p->p_semAdd = semAdd;
        insertProcQ(&iterator->s_procq,p);
        return 0;
    }

    if (!list_empty(&semdFree_h)) {
        semd_t* semdToAdd = list_first_entry(&semdFree_h,semd_t,s_freelink);
    
        /* node initialization */
        semdToAdd->s_key = semAdd;
        mkEmptyProcQ(&semdToAdd->s_procq); 
        p->p_semAdd = semAdd; 
        insertProcQ(&semdToAdd->s_procq,p);

        hash_add(semd_h,&semdToAdd->s_link, semdToAdd->s_key);
        list_del(&semdToAdd->s_freelink);
        return 0;
    }
    return 1;
}

pcb_t* removeBlocked(int *semAdd){
    pcb_t* pcbToReturn = NULL;
    semd_t* iterator;
    struct hlist_node* tmp = NULL;
    hash_for_each_possible_safe(semd_h,iterator,tmp,s_link,semAdd){ 
        pcbToReturn = removeProcQ(&iterator->s_procq);    
        pcbToReturn->p_semAdd = NULL;  

        removeEmptySemd(iterator); 
    }
    return pcbToReturn;
}

pcb_t* outBlocked(pcb_t* p){
    semd_t* iterator;
    struct hlist_node* tmp=NULL;

    pcb_t* pcbToReturn = NULL;
    hash_for_each_possible_safe(semd_h,iterator,tmp,s_link,(p->p_semAdd)){
        pcbToReturn = outProcQ(&iterator->s_procq,p);

        removeEmptySemd(iterator);
    }
    return pcbToReturn;
}

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

