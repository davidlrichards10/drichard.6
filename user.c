
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>	
#include <semaphore.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <time.h>
#include "shared.h"

/* for shared memory and semaphore */
int shmid; 
sm* ptr;
sem_t *sem;

int messageQ;

struct message {
    long msgType;
    char mtext[512];
}msg;

void incClock(struct time* time, int sec, int ns);

int main(int argc, char* argv[]) 
{
	/* get shared memory */
     	if ((shmid = shmget(9784, sizeof(sm), 0600)) < 0) 
	{
            perror("Error: shmget");
            exit(errno);
     	}
	
	if ((messageQ = msgget(4020015, 0777 )) == -1 ) 
	{
        	perror("USER: Error generating message queue");
        	exit(0);
    	}
	
	/*open semaphore used to protect the clock */
	sem = sem_open("p5sem", 0);   

	/* attach to shared memory */
	ptr = shmat(shmid, NULL, 0);

	time_t t;
	time(&t);	
	srand((int)time(&t) % getpid());

	while(1) 
	{
			strcpy(msg.mtext,"TERMINATED");
			msg.msgType = 99;
			msgsnd(messageQ,&msg,sizeof(msg),0);
			exit(0);	
	}	
	return 0;
}

/* function to increment the clock and is protected via semaphore */
void incClock(struct time* time, int sec, int ns)
{
	sem_wait(sem);
	time->seconds += sec;
	time->nanoseconds += ns;
	while(time->nanoseconds >= 1000000000)
	{
		time->nanoseconds -=1000000000;
		time->seconds++;
	}
	sem_post(sem);
}
