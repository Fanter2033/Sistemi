#include "ash.h"

/* Active Semaphore Hash*/
HIDDEN DECLARE_HASHTABLE(semd_h,5);

/* List of free or unused SEMD */
HIDDEN LIST_HEAD(semdFree_h);


void removeEmptySemd(semd_t* s){
    if (emptyProcQ(&s->s_procq)){
        hash_del(&(s->s_link));
        list_add(&(s->s_freelink),&semdFree_h);
    }
}

int insertBlocked(int *semAdd, pcb_t *p){
    if (p->p_semAdd!=NULL) return 1;    //case: p is already blocked
    /* search for semAdd as key in hash */
    struct semd_t* iterator; 
    int bkt;
    hash_for_each(semd_h, bkt, iterator,s_link){
        if (iterator -> s_key == semAdd){
            p->p_semAdd = semAdd;
            insertProcQ(&iterator->s_procq,p);
            return 0;
        }
    }
    /* adding new semaphore */
    if (!list_empty(&semdFree_h)) {
        semd_t* semdToAdd = list_first_entry(&semdFree_h,semd_t,s_freelink);
    
        /* node initialization */
        semdToAdd->s_key = semAdd;
        mkEmptyProcQ(&semdToAdd->s_procq); 
        p->p_semAdd = semAdd; 
        insertProcQ(&semdToAdd->s_procq,p);

        hash_add(semd_h,&semdToAdd->s_link, (u32)semdToAdd->s_key);
        list_del(&semdToAdd->s_freelink);
        return 0;
    }
    return 1;
}

pcb_t* removeBlocked(int *semAdd){
    pcb_t* pcbToReturn = NULL;
    semd_t* iterator;
    int bkt;
    hash_for_each(semd_h,bkt,iterator,s_link){ 
        if (iterator -> s_key == semAdd){
            pcbToReturn = removeProcQ(&iterator->s_procq);    
            pcbToReturn->p_semAdd = NULL;  

            removeEmptySemd(iterator); 
            return pcbToReturn;
        }

    }
    /* pcbToReturn is NULL when the semaphore's key isn't in hash*/
    return NULL;
}

pcb_t* outBlocked(pcb_t* p){
    semd_t* iterator;
    pcb_t* pcbToReturn = NULL;
    int  bkt;

    hash_for_each(semd_h,bkt,iterator,s_link){
        if (iterator -> s_key == p -> p_semAdd){
            pcbToReturn = outProcQ(&iterator->s_procq,p);
            pcbToReturn->p_semAdd = NULL;

            removeEmptySemd(iterator);
            return pcbToReturn;
        }
    }
    /* pcbToReturn is NULL when the semaphore's key isn't in hash*/
    return NULL;
}

pcb_t* headBlocked(int *semAdd){
    semd_t* iterator;
    int bkt;
    hash_for_each(semd_h,bkt,iterator,s_link){
        if (iterator -> s_key == semAdd){
            return headProcQ(&iterator->s_procq);
        }
    }
    /* pcbToReturn is NULL when the semaphore's key isn't in hash*/
    return NULL;
}

void initASH(){
    static semd_t semd_table[MAXPROC];
    for (int i=0;i<MAXPROC;i++)
        list_add(&(semd_table[i].s_freelink),&semdFree_h);
}
