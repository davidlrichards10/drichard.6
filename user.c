
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
	
	time_t t;
        time(&t);       
        srand((int)time(&t) % getpid());
	
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

	/* set time for when a process should either request or release */
	int nextMove = (rand() % 1000000 + 1);
	struct time moveTime;
	sem_wait(sem);
	moveTime.seconds = ptr->time.seconds;
	moveTime.nanoseconds = ptr->time.nanoseconds;
	sem_post(sem);

	/* set time for a when a process should check if its time to terminate */
	int termination = (rand() % (250 * 1000000) + 1);
	struct time termCheck;
	sem_wait(sem);
	termCheck.seconds = ptr->time.seconds;
	termCheck.nanoseconds = ptr->time.nanoseconds;
	sem_post(sem);

	if(ptr->resourceStruct.memType == 0)
	{
		while(1) 
		{

		/* if its time for the next action, check whether to request or release */
			if((ptr->time.seconds > moveTime.seconds) || (ptr->time.seconds == moveTime.seconds && ptr->time.nanoseconds >= moveTime.nanoseconds))
			{
				/* set time for next acion check */
                                sem_wait(sem);
                                moveTime.seconds = ptr->time.seconds;
                                moveTime.nanoseconds = ptr->time.nanoseconds;
                                sem_post(sem);
                                incClock(&moveTime, 0, nextMove);
				
				if(rand()%100 < 40)
				{
					strcpy(msg.mtext,"REQUEST");
					msg.msgType = 99;
					msgsnd(messageQ,&msg,sizeof(msg),0);
				
					int request = rand() % (30 + 1) + 1;
					//ptr->resourceStruct.request = request;
					sprintf(msg.mtext,"%d",request);
                                        msgsnd(messageQ,&msg,sizeof(msg),0);
				}
				else
				{
					strcpy(msg.mtext,"WRITE");
                                        msg.msgType = 99;
                                        msgsnd(messageQ,&msg,sizeof(msg),0);
					int write = rand() % (30 + 1) +1;
					//ptr->resourceStruct.write = write;		
					sprintf(msg.mtext,"%d",write);
					msgsnd(messageQ,&msg,sizeof(msg),0);
				}
			}


			if((ptr->resourceStruct.count % 100) == 0 && ptr->resourceStruct.count!=0)
			{
				if((rand()%100) <= 70)
				{
					strcpy(msg.mtext,"TERMINATED");
					msg.msgType = 99;
					msgsnd(messageQ,&msg,sizeof(msg),0);
				}
			}

			exit(0);
			
		}	
	}
	else
	{
		while(1)
		{
			exit(0);
		}
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
