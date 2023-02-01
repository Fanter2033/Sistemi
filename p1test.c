/*********************************P1TEST.C*******************************
 *
 *	Test program for the modules ASH and pcbQueues (phase 1).
 *
 *	Produces progress messages on terminal 0 in addition
 *		to the array ``okbuf[]''
 *		Error messages will also appear on terminal 0 in
 *		addition to the array ``errbuf[]''.
 *
 *		Aborts as soon as an error is detected.
 *
 *      Modified by Michael Goldweber on May 15, 2004
 *      Modified by Davide Berardi on November 23, 2022
 */

#include "./pandos_const.h"
#include "./pandos_types.h"

#include <umps3/umps/libumps.h>
#include "./pcb.h"
#include "./ash.h"


#define MAXPROC 20
#define MAXSEM  MAXPROC
#define MAXNS   MAXPROC

char   okbuf[2048]; /* sequence of progress messages */
char   errbuf[128]; /* contains reason for failing */
char   msgbuf[128]; /* nonrecoverable error message before shut down */
int    sem[MAXSEM];
int    onesem;
pcb_t *procp[MAXPROC], *p, *q, *firstproc, *lastproc, *midproc;
nsd_t *pid_ns, *pid_ns2;
char  *mp = okbuf;


#define TRANSMITTED 5
#define ACK         1
#define PRINTCHR    2
#define CHAROFFSET  8
#define STATUSMASK  0xFF
#define TERM0ADDR   0x10000254

typedef unsigned int devreg;

/* This function returns the terminal transmitter status value given its address */
devreg termstat(memaddr *stataddr) {
    return ((*stataddr) & STATUSMASK);
}

/* This function prints a string on specified terminal and returns TRUE if
 * print was successful, FALSE if not   */
unsigned int termprint(char *str, unsigned int term) {
    memaddr     *statusp;
    memaddr     *commandp;
    devreg       stat;
    devreg       cmd;
    unsigned int error = FALSE;

    if (term < DEVPERINT) {
        /* terminal is correct */
        /* compute device register field addresses */
        statusp  = (devreg *)(TERM0ADDR + (term * DEVREGSIZE) + (TRANSTATUS * DEVREGLEN));
        commandp = (devreg *)(TERM0ADDR + (term * DEVREGSIZE) + (TRANCOMMAND * DEVREGLEN));

        /* test device status */
        stat = termstat(statusp);
        if (stat == READY || stat == TRANSMITTED) {
            /* device is available */

            /* print cycle */
            while (*str != EOS && !error) {
                cmd       = (*str << CHAROFFSET) | PRINTCHR;
                *commandp = cmd;

                /* busy waiting */
                stat = termstat(statusp);
                while (stat == BUSY)
                    stat = termstat(statusp);

                /* end of wait */
                if (stat != TRANSMITTED)
                    error = TRUE;
                else
                    /* move to next char */
                    str++;
            }
        } else
            /* device is not available */
            error = TRUE;
    } else
        /* wrong terminal device number */
        error = TRUE;

    return (!error);
}

/* This function placess the specified character string in okbuf and
 *	causes the string to be written out to terminal0 */
void addokbuf(char *strp) {
    char *tstrp = strp;
    while ((*mp++ = *strp++) != '\0')
        ;
    mp--;
    termprint(tstrp, 0);
}


/* This function placess the specified character string in errbuf and
 *	causes the string to be written out to terminal0.  After this is done
 *	the system shuts down with a panic message */
void adderrbuf(char *strp) {
    char *ep    = errbuf;
    char *tstrp = strp;

    while ((*ep++ = *strp++) != '\0')
        ;

    termprint(tstrp, 0);

    PANIC();
}

int main(void) {

    int i;

    initPcbs();
    addokbuf("Initialized process control blocks   \n");
    
    /* check ASH */
    initASH();
    addokbuf("Initialized active semaphore hash   \n");

    
    /* check removeBlocked and insertBlocked */
    addokbuf("insertBlocked test #1 started  \n");
    for (i = 10; i < MAXPROC; i++) {
        procp[i] = allocPcb();
        if (insertBlocked(&sem[i], procp[i]))
            adderrbuf("insertBlocked(1): unexpected TRUE   ");
    }

    addokbuf("insertBlocked test #2 started  \n");
    for (i = 0; i < 10; i++) {
        procp[i] = allocPcb();
        if (insertBlocked(&sem[i], procp[i]))
            adderrbuf("insertBlocked(2): unexpected TRUE   ");
    }
    
    /* check if semaphore descriptors are returned to free list */
    addokbuf("insert OKKKK\n");

    p = removeBlocked(&sem[11]);
    if (insertBlocked(&sem[11], p))
        adderrbuf("removeBlocked: fails to return to free list   ");

    if (insertBlocked(&onesem, procp[9]) == FALSE)
        adderrbuf("insertBlocked: inserted more than MAXPROC   ");

    addokbuf("removeBlocked test started   \n");
    for (i = 10; i < MAXPROC; i++) {
        q = removeBlocked(&sem[i]);
        if (q == NULL)
            adderrbuf("removeBlocked: wouldn't remove   ");
        if (q != procp[i])
            adderrbuf("removeBlocked: removed wrong element   ");
        if (insertBlocked(&sem[i - 10], q))
            adderrbuf("insertBlocked(3): unexpected TRUE   ");
    }
    if (removeBlocked(&sem[11]) != NULL)
        adderrbuf("removeBlocked: removed nonexistent blocked proc   ");
    
    addokbuf("insertBlocked and removeBlocked ok   \n");

    return 0;


    }
