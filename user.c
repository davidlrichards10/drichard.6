/* Name: David Richards
 * Date: Tue, May 5th
 * Assignment: hw6
 * file: user.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
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
float pageWeight;
float processWeightBound;

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
            exit(0);
     	}
	
	if ((messageQ = msgget(4020015, 0777 )) == -1 ) 
	{
        	perror("Error: message queue");
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
				
				/* see if a process needs to request read/write and generate random address */
				if(rand()%100 < 60)
				{
					strcpy(msg.mtext,"REQUEST");
					msg.msgType = 99;
					msgsnd(messageQ,&msg,sizeof(msg),0);
				
					int request = rand() % (31998 + 1) + 1;
					sprintf(msg.mtext,"%d",request);
                                        msgsnd(messageQ,&msg,sizeof(msg),0);
				}
				else
				{
					strcpy(msg.mtext,"WRITE");
                                        msg.msgType = 99;
                                        msgsnd(messageQ,&msg,sizeof(msg),0);
					int write = rand() % (31998 + 1) +1;	
					sprintf(msg.mtext,"%d",write);
					msgsnd(messageQ,&msg,sizeof(msg),0);
				}
			}

			/* check for termination every 1000 memory references */
			if((ptr->resourceStruct.count % 1000) == 0 && ptr->resourceStruct.count!=0)
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
	/* method 2 for memory access */
	if(ptr->resourceStruct.memType == 1)
	{
		while(1)
		{
			/* check if its time for next action and advance clock accordingly */
                        if((ptr->time.seconds > moveTime.seconds) || (ptr->time.seconds == moveTime.seconds && ptr->time.nanoseconds >= moveTime.nanoseconds))
                        {
                                sem_wait(sem);
                                moveTime.seconds = ptr->time.seconds;
                                moveTime.nanoseconds = ptr->time.nanoseconds;
                                sem_post(sem);
                                incClock(&moveTime, 0, nextMove);
			/* initialize array where weight is 1/n */
			float processWeight;
			int i;
			/* add to each index of the array the value in the preceding index */
			for(i = 0; i < 31; i++)
			{
				ptr->resourceStruct.weights[0] = 1;
				ptr->resourceStruct.weights[i + 1] = 1 + ptr->resourceStruct.weights[i + 1];
			}
			processWeightBound = ptr->resourceStruct.weights[i];
			
			/* generate a random number from 0 to the last value in the array */
			int num = (rand() % (int)processWeightBound + 1);
			int j;
	
			/* travel down the array until you find a value greater than that num */ 
			for(j = 0; j < 32; j++)
			{
				if(ptr->resourceStruct.weights[j] > num)
				{
					pageWeight = ptr->resourceStruct.weights[j];
					break;
				}
			}

			/* multiply age number by 1024 and add random offset to get the actual memory address */
			float pageNumber = pageWeight * 24;
			float randomOffset = rand() % 1023;
				/* see if it needs to be a read or write request */
				if(rand()%100 < 60)
				{
					strcpy(msg.mtext,"REQUEST");
					msg.msgType = 99;
					msgsnd(messageQ,&msg,sizeof(msg),0);
					int methodTwoRequest = pageNumber + randomOffset;
					sprintf(msg.mtext,"%d",methodTwoRequest);
                                        msgsnd(messageQ,&msg,sizeof(msg),0);
				}
				else
				{
					strcpy(msg.mtext,"WRITE");
                                        msg.msgType = 99;
                                        msgsnd(messageQ,&msg,sizeof(msg),0);
					int methodTwoRequest = pageNumber + randomOffset;
					sprintf(msg.mtext,"%d",methodTwoRequest);
					msgsnd(messageQ,&msg,sizeof(msg),0);
				}
			}


			/* check for termination every 1000 memory references */
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
