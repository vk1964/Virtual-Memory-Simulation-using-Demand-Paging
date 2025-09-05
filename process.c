#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/msg.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <ctype.h>
#include <sys/shm.h>

typedef struct {
    long mtype;
    int id;
    int pageno;
}mmu_send;

typedef struct {
    long mtype;
    int frameno;
}mmu_recv;

typedef struct{
    long mtype;
    int id;
}mymsg;

int main(int argc, char *argv[])
{
    if(argc != 5)
    {
        printf("Inavalid arguments\n");
        exit(1);
    }
    int q1_id = atoi(argv[1]);
    int q3_id = atoi(argv[2]);
    int id = atoi(argv[3]);
    int n = 0;
    int i = 0;
    while(argv[4][i] != '\0')
    {
        if(argv[4][i] == ' ') n++;
        i++;
    }
    //printf("Process %d has %d pages\n", id, n);
    int pages[n-1];
    i = 0;
    char *token = strtok(argv[4], " ");
    while(token != NULL)
    {
        pages[i] = atoi(token);
        token = strtok(NULL, " ");
        i++;
    }
    mymsg msg;
    msg.mtype = 1;
    msg.id = id;
    msgsnd(q1_id, &msg, sizeof(msg), 0);
    msgrcv(q1_id, &msg, sizeof(msg), 2 + id, 0);
    printf("Process %d started\n", id);
    mmu_recv m;
    mmu_send m1;
    int finished = 0;
    i = 0;
    while(finished < n){
        m1.mtype = 1;
        m1.id = id;
        m1.pageno = pages[finished];
        msgsnd(q3_id, &m1, sizeof(m1), 0);
        msgrcv(q3_id, &m, sizeof(m), 2, 0);
        //printf("Page %d of process %d is in frame %d\n", pages[finished], id, m.frameno);
        if(m.frameno >= 0){
            //printf("Page %d of process %d is in frame %d\n", pages[finished], id, m.frameno);
            finished++;
        }
        else if(m.frameno == -1){
            //printf("Page %d of process %d is not in memory and page fault occured\n", pages[finished], id);
            memset(&msg, 0, sizeof(msg));
            msgrcv(q1_id, &msg, sizeof(msg), 2 + id, 0);
            //printf("Process %d started\n", id);
        }
        else if(m.frameno == -2){
            //printf("Page %d of process %d is not in memory and invalid memory access\n", pages[finished], id);
            printf("Process %d terminated\n", id);
            exit(1);
        }
    }
    m1.mtype = 1;
    m1.id = id;
    m1.pageno = -9;
    msgsnd(q3_id, &m1, sizeof(m1), 0);
    printf("Process %d terminated\n", id);
    exit(0);
}

