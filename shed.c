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
}mymsg;

typedef struct {
    long mtype;
    int mes;
}mmu_recv;

int main(int argc, char *argv[])
{
    if(argc != 5)
    {
        printf("Inavalid arguments\n");
        exit(1);
    }
    int q1_id = atoi(argv[1]);
    int q2_id = atoi(argv[2]);
    int k = atoi(argv[3]);
    int mp = atoi(argv[4]);
    int terminated = 0;
    mymsg msg;
    mmu_recv mmsg;
    while(terminated < k)
    {
        msgrcv(q1_id, &msg, sizeof(msg), 1, 0);
        int curr = msg.id;
        msg.mtype = 2+curr;
        //printf("Sending signal for process %d\n", curr);
        msgsnd(q1_id, &msg, sizeof(msg), 0);
        msgrcv(q2_id, &mmsg, sizeof(mmsg), 1, 0);
        if(mmsg.mes == 1)
        {
            terminated++;
            continue;
        }
        if(mmsg.mes == 0)
        {
            //printf("Page fault handled for %d\n", curr);
            msg.mtype = 1;
            msg.id = curr;
            msgsnd(q1_id, &msg, sizeof(msg), 0);
        }
    }
    printf("All processes terminated\n");
    //printf("Sending signal to MMU %d\n", mp);
    kill(mp, SIGINT);
    exit(0);
}
