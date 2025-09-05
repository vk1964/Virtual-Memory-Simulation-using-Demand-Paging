#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <limits.h>
#include <math.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define PROB 0.001

int flag = 1;

typedef struct {
    int fn;
    int c;
    int v;
}ptable;

typedef struct {
    int max;
    int f_max;
    int f_alloc;
    int is_terminated;
}ppmap;

typedef struct {
    int pos;
    int free[];
}freelist;

void sighandler()
{
    //printf("\nSignal received\n");
    flag = 0;
}

void print_ppmap(ppmap *pp, int k)
{
    printf("Process\tMax\tAlloc\tMax Frames\n");
    for(int i=0;i<k;i++)
    {
        printf("%d\t%d\t%d\t%d\n",i,pp[i].max,pp[i].f_alloc,pp[i].f_max);
    }
}

int main()
{
    signal(SIGINT, sighandler);
    srand(time(0));
    int k,m,f,shed,mmu;
    printf("Enter number of processes to be created: ");
    scanf("%d", &k);
    printf("Enter maximum number of pages per process: ");
    scanf("%d", &m);
    printf("Enter number of frames: ");
    scanf("%d", &f);
    if(k <= 0 || m <= 0 || f <=0 || f < k)
	{
		printf("Invalid input\n");
		exit(1);
	}
    key_t free_key, ptable_key, ppmap_key, q1_key, q2_key, q3_key;
    free_key = ftok(".",1000);
    ptable_key = ftok(".",2000);
    ppmap_key = ftok(".",3000);
    q1_key = ftok(".",4000);
    q2_key = ftok(".",5000);
    q3_key = ftok(".",6000);
    int free_id, ptable_id, ppmap_id, q1_id, q2_id, q3_id;
    free_id = shmget(free_key, sizeof(freelist) + f*sizeof(int), IPC_CREAT | 0666);
    freelist *fl = (freelist *)shmat(free_id, NULL, 0);
    fl->pos = 0;
    for(int i=0; i<f; i++)
        fl->free[i] = i;
    shmdt(fl);
    ptable_id = shmget(ptable_key, m*sizeof(ptable)*k, IPC_CREAT | 0666);
    ptable *pt = (ptable *)shmat(ptable_id, NULL, 0);
    for(int i=0; i<k; i++)
        for(int j=0; j<m; j++)
        {
            pt[i*m+j].fn = -1;
            pt[i*m+j].c = 0;
            pt[i*m+j].v = 0;
        }
    shmdt(pt);
    ppmap_id = shmget(ppmap_key, k*sizeof(ppmap), IPC_CREAT | 0666);
    ppmap *pp = (ppmap *)shmat(ppmap_id, NULL, 0);
    int totpages = 0;
    for(int i=0;i<k;i++){
        pp[i].max = rand()%m + 1;
        pp[i].f_max = 0;
        pp[i].f_alloc = 0;
        pp[i].is_terminated = 0;
        totpages += pp[i].max;
    }
    int max = 0, maxi = 0;
    int alloc = 0;
    for(int i=0;i<k;i++){
        if(pp[i].max > max)
        {
            max = pp[i].max;
            maxi = i;
        }
        pp[i].f_max = floor((float)pp[i].max*f/totpages);
        if(pp[i].f_max == 0)
            pp[i].f_max = 1;
        alloc += pp[i].f_max;
    }
    pp[maxi].f_max += f - alloc;
    //print_ppmap(pp,k);
    q1_id = msgget(q1_key, IPC_CREAT | 0666);
    q2_id = msgget(q2_key, IPC_CREAT | 0666);
    q3_id = msgget(q3_key, IPC_CREAT | 0666);
    printf("Shared memory and message queues created\n");
    if((shed = fork()) == 0)
    {
        char b1[20],b2[20],b3[20],b4[20];
        memset(b1,0,20);
        memset(b2,0,20);
        memset(b3,0,20);
        memset(b4,0,20);
        sprintf(b1, "%d", q1_id);
        sprintf(b2, "%d", q2_id);
        sprintf(b3, "%d", k);
        sprintf(b4, "%d", getppid());
        execlp("./sheduler","./sheduler",b1,b2,b3,b4,NULL);
        printf("Error in creating sheduler\n");
        exit(1);
    }
    if((mmu = fork()) == 0)
    {
        char b1[20],b2[20],b3[20],b4[20],b5[20],b6[20],b7[20],b8[20];
        memset(b1,0,20);
        memset(b2,0,20);
        memset(b3,0,20);
        memset(b4,0,20);
        memset(b5,0,20);
        memset(b6,0,20);
        memset(b7,0,20);
        memset(b8,0,20);
        sprintf(b1, "%d", q2_id);
        sprintf(b2, "%d", q3_id);
        sprintf(b3, "%d", free_id);
        sprintf(b4, "%d", ptable_id);
        sprintf(b5, "%d", ppmap_id);
        sprintf(b6, "%d", m);
        sprintf(b7, "%d", f);
        sprintf(b8, "%d", k);
        execlp("xterm","xterm","-T","MMU","-e","./mmu",b1,b2,b3,b4,b5,b6,b7,b8,NULL);
        //execlp("./mmu","./mmu",b1,b2,b3,b4,b5,b6,b7,b8,NULL);
        printf("Error in creating mmu\n");
        exit(1);
    }
    printf("Creating processes\n");
    for(int i=0;i<k;i++)
    {
        int rlen = rand()%(8*pp[i].max + 1) + 2*pp[i].max;
        char rstring[m*20*40];
        int len = 0;
        for(int j=0;j<rlen;j++){
            int r = rand()%pp[i].max;
            float p = (float)(rand() % 1000)/1000;
            if(p <PROB){
                r += pp[i].max;
            }
            len += sprintf(rstring+len,"%d ",r);
        }
        //printf("Process %d: %s\n",i,rstring);
        if(fork() == 0)
        {
            char b1[20],b2[20],b3[20];
            memset(b1,0,20);
            memset(b2,0,20);
            memset(b3,0,20);
            sprintf(b1, "%d", q1_id);
            sprintf(b2, "%d", q3_id);
            sprintf(b3, "%d", i);
            execlp("./process","./process",b1,b2,b3,rstring,NULL);
            printf("Error in creating process %d\n",i);
            exit(1);
        }
        usleep(250*1000);
    }
    while(flag);
    kill(shed, SIGKILL);
    waitpid(shed, NULL, 0);
    waitpid(mmu, NULL, 0);
    printf("\nTerminated subroutines\n");
    shmdt(pp);
    shmctl(ppmap_id, IPC_RMID, NULL);
    shmdt(pt);
    shmctl(ptable_id, IPC_RMID, NULL);
    shmdt(fl);
    shmctl(free_id, IPC_RMID, NULL);
    msgctl(q1_id, IPC_RMID, NULL);
    msgctl(q2_id, IPC_RMID, NULL);
    msgctl(q3_id, IPC_RMID, NULL);
    return 0;
}
