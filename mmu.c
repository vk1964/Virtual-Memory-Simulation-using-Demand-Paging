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

int flag = 1;
int count = 0;

void sighandler()
{
    flag = 0;
}

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

typedef struct {
    long mtype;
    int id;
    int pageno;
}pr_recv;

typedef struct {
    long mtype;
    int frameno;
}pr_send;

typedef struct {
    long mtype;
    int mes;
}sh_send;

typedef struct {
    int faults;
    int invalid;
} logistics;

void print_ppmap(ppmap *pp, int k)
{
    printf("Process\tMax\tAlloc\tMax Frames\n");
    for(int i=0;i<k;i++)
    {
        printf("%d\t%d\t%d\t%d\n",i,pp[i].max,pp[i].f_alloc,pp[i].f_max);
    }
}

int main(int argc, char *argv[])
{
    if(argc != 9)
    {
        printf("Invalid arguments\n");
        exit(1);
    }
    FILE *file = fopen("result.txt", "w");
    signal(SIGINT, sighandler);
    signal(SIGUSR1, sighandler);
    int q2_id = atoi(argv[1]);
    int q3_id = atoi(argv[2]);
    int free_id = atoi(argv[3]);
    int ptable_id = atoi(argv[4]);
    int ppmap_id = atoi(argv[5]);
    int m = atoi(argv[6]);
    int f = atoi(argv[7]);
    int k = atoi(argv[8]);
    logistics stats[k];
    for(int i=0;i<k;i++)
    {
        stats[i].faults = 0;
        stats[i].invalid = 0;
    }
    freelist *fl = (freelist *)shmat(free_id, NULL, 0);
    ptable *pt = (ptable *)shmat(ptable_id, NULL, 0);
    ppmap *pp = (ppmap *)shmat(ppmap_id, NULL, 0);
    pr_recv m1;
    pr_send m2;
    sh_send m3;
    int terminated = 0;
    while(terminated < k)
    {
        msgrcv(q3_id, &m1, sizeof(m1), 1, 0);
        int curr = m1.id;
        int pno = m1.pageno;
        if(pno == -9){
            terminated++;
            pp[curr].is_terminated = 1;
            pp[curr].f_max = 0;
            for(int i=0;i<m;i++)
            {
                if(pt[curr*m + i].v)
                {
                    fl->free[--fl->pos] = pt[curr*m + i].fn;
                    pt[curr*m + i].v = 0;
                    pp[curr].f_alloc--;
                }
            }
            int tot_pages = 0;
            for(int i=0;i<k;i++)
            {
                if(!pp[i].is_terminated)
                    tot_pages += pp[i].max;
            }
            if(tot_pages){
                int max = 0,maxi = 0;
                int alloc = 0;
                for(int i=0;i<k;i++)
                {
                    if(!pp[i].is_terminated)
                    {
                        if(pp[i].max > max)
                        {
                            max = pp[i].max;
                            maxi = i;
                        }
                        pp[i].f_max = (int)floor((f*pp[i].max)/tot_pages);
                        if(pp[i].f_max == 0)
                            pp[i].f_max = 1;
                        alloc += pp[i].f_max;
                    }
                }
                pp[maxi].f_max += f - alloc;
                //print_ppmap(pp,k);
            }
            m3.mtype = 1;
            m3.mes = 1;
            msgsnd(q2_id, &m3, sizeof(m3), 0);
            continue;
        }
        count++;
        printf("time: %d, Process: %d, Page: %d:\n", count, curr, pno);
        fprintf(file, "time: %d, Process: %d, Page: %d:\n", count, curr, pno);
        if(pno >= pp[curr].max)
        {
            printf("\t\tInvalid memory access\n");
            fprintf(file, "\t\tInvalid memory access\n");
            m3.mtype = 1;
            m3.mes = 1;
            msgsnd(q2_id, &m3, sizeof(m3), 0);
            m2.mtype = 2;
            m2.frameno = -2;
            msgsnd(q3_id, &m2, sizeof(m2), 0);
            stats[curr].invalid++;
            terminated++;
            pp[curr].is_terminated = 1;
            pp[curr].f_max = 0;
            for(int i=0;i<m;i++)
            {
                if(pt[curr*m + i].v)
                {
                    fl->free[--fl->pos] = pt[curr*m + i].fn;
                    pt[curr*m + i].v = 0;
                    pp[curr].f_alloc--;
                }
            }
            int tot_pages = 0;
            for(int i=0;i<k;i++)
            {
                if(!pp[i].is_terminated)
                    tot_pages += pp[i].max;
            }
            if(tot_pages){
                int max = 0,maxi = 0;
                int alloc = 0;
                for(int i=0;i<k;i++)
                {
                    if(!pp[i].is_terminated)
                    {
                        if(pp[i].max > max)
                        {
                            max = pp[i].max;
                            maxi = i;
                        }
                        pp[i].f_max = (int)floor((f*pp[i].max)/tot_pages);
                        if(pp[i].f_max == 0)
                            pp[i].f_max = 1;
                        alloc += pp[i].f_max;
                    }
                }
                pp[maxi].f_max += f - alloc;
                //print_ppmap(pp,k);
            }
            continue;
        }
        if(pt[curr*m + pno].v){
            printf("\t\tPage found in frame %d\n", pt[curr*m + pno].fn);
            fprintf(file, "\t\tPage found in frame %d\n", pt[curr*m + pno].fn);
            m2.mtype = 2;
            m2.frameno = pt[curr*m + pno].fn;
            pt[curr*m + pno].c = count;
            msgsnd(q3_id, &m2, sizeof(m2), 0);
        }
        else{
            printf("\t\tPage fault\n");
            fprintf(file, "\t\tPage fault\n");
            stats[curr].faults++;
            m2.mtype = 2;
            m2.frameno = -1;
            msgsnd(q3_id, &m2, sizeof(m2), 0);
            if(fl->pos == f || pp[curr].f_alloc == pp[curr].f_max)
            {
                int min = INT_MAX;
                int min_id = -1;
                for(int i=0;i<m;i++){
                    if(pt[curr*m + i].v && pt[curr*m + i].c < min){
                        min = pt[curr*m + i].c;
                        min_id = i;
                    }
                }
                if(min_id == -1)
                {
                    printf("Error\n");
                    exit(1);
                }
                pt[curr*m + min_id].v = 0;
                pt[curr*m + pno].fn = pt[curr*m + min_id].fn;
                pt[curr*m + pno].v = 1;
                pt[curr*m + pno].c = count;
            }
            else{
                pt[curr*m + pno].fn = fl->free[fl->pos];
                fl->pos++;
                pt[curr*m + pno].v = 1;
                pt[curr*m + pno].c = count;
                pp[curr].f_alloc++;
            }
            m3.mtype = 1;
            m3.mes = 0;
            msgsnd(q2_id, &m3, sizeof(m3), 0);
        }
    }
    printf("Statistics:\n");
    fprintf(file, "Statistics:\n");
    for(int i=0;i<k;i++)
    {
        printf("\tProcess %d\n", i);
        fprintf(file, "\tProcess %d\n", i);
        printf("\t\tPage faults: %d\n", stats[i].faults);
        fprintf(file, "\t\tPage faults: %d\n", stats[i].faults);
        printf("\t\tInvalid memory access: %d\n", stats[i].invalid);
        fprintf(file, "\t\tInvalid memory access: %d\n", stats[i].invalid);
    }
    fflush(file);
    fflush(stdout);
    printf("Enter any key to exit\n");
    getc(stdin);
    //sleep(5);
    fclose(file);
    shmdt(fl);
    shmdt(pt);
    shmdt(pp);
    exit(0);
}
