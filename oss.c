
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h> 
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <time.h>
#include "shared.h"

/* for shared memory setup/semaphore */
int shmid; 
sm* ptr;
sem_t *sem;

struct message {
    long msgType;
    char mtext[512];
};

struct message msg;

int messageQ;
int stillActive[20];
int pidNum = 0;
int termed = 0;	

void setUp();
void detach();
void sigErrors();
void incClock(struct time* time, int sec, int ns);

int timer = 2;
int memoryAccess = 0;

/* default output file */
char outputFileName[] = "log";
FILE* fp;

int main(int argc, char* argv[]) 
{
	/* getopt to parse command line options */
	int c;
	int i = 0;
	while((c=getopt(argc, argv, "m:i:t:h"))!= EOF)
	{
		switch(c)
		{
			case 'h':
				printf("\nInvocation: oss [-h] [-i x -m x -t x]\n");
                		printf("----------------------------------------------Program Options-------------------------------------------\n");
                		printf("       -h             Describe how the project should be run and then, terminate\n");
				printf("       -i             Type file name to print program information in (Default of log)\n");
				printf("       -m             Indicate memory access type with 0 or 1 (Default 0)\n");
				printf("       -t             Indicate timer amount (Default of 2 seconds)\n");				
				exit(0);
				break;
			case 'm':
				memoryAccess = atoi(optarg);
				break;
			case 'i':
                		strcpy(outputFileName, optarg);
                		break;
			case 't':
				timer = atoi(optarg);
				break;
			default:
				return -1;
		}
	}

	fp = fopen(outputFileName, "w");
	
	int maxPro = 100;
	srand(time(NULL));
	int count = 0;	

	pid_t cpid;

	int pid = 0;

 	time_t t;
	srand((unsigned) time(&t));
	int totalCount = 0;

	/* all processes start out as active when spawned */		
	for(i = 0; i < 18; i++)
	{
		stillActive[i] = i;
	}

	/* Catch ctrl-c and 3 second alarm interupts */
	if (signal(SIGINT, sigErrors) == SIG_ERR) //sigerror on ctrl-c
        {
                exit(0);
        }

        if (signal(SIGALRM, sigErrors) == SIG_ERR) //sigerror on program exceeding specified second alarm
        {
                exit(0);
        }

	setUp();
	
	if(memoryAccess == 0)
	{
		printf("Memory access method 0\n");
		ptr->resourceStruct.memType = 0;
	}
	else
	{
		printf("Memory access method 1\n");
		ptr->resourceStruct.memType = 1;
	}
	struct time randFork;

	/* start alarm based on user specification */
	alarm(timer);

	/* run for a max of 100 processes or no process are remaining alive */
	while(totalCount < maxPro || count > 0)
	{ 							
		
			if(waitpid(cpid,NULL, WNOHANG)> 0)
			{
				count--;
			}

			/* increment the clock by 70,000 per turn and by the initial fork */
			incClock(&ptr->time,0,70000);
			int nextFork = (rand() % (500000000 - 1000000 + 1)) + 1000000;
			incClock(&randFork,0,nextFork);
			
			/* run as long as there are less that 18 processes running at once */
			if(count < 18 && ptr->time.nanoseconds < nextFork) 
			{
						/* next fork gets changed each run through this loop */
						sem_wait(sem);
						randFork.seconds = ptr->time.seconds;
						randFork.nanoseconds = ptr->time.nanoseconds;
						sem_post(sem);
						nextFork = (rand() % (500000000 - 1000000 + 1)) + 1000000;
						incClock(&randFork,0,nextFork);
			
						/* run until all processes terminate normally */
						int l;
        					for(l=0; l<18;l++)
						{
                					if(stillActive[l] == -1)
							{
                        					termed++;
                					}
        					}

						/* exit and detach shared memory once all process terminate */
        					if(termed == 18)
						{	
							detach();
							return 0;

                				} 
						else 
						{
                        				termed = 0;
                				}

        					if(stillActive[pidNum] != -1)
						{
                					pid = stillActive[pidNum];
                				} 
						else 
						{
                					int s = pidNum;
                					for(s=pidNum; s<18;s++)
							{
                						if(stillActive[s] == -1)
								{
                        						pidNum++;
                						} 
								else 
								{
                							break;
                						}

                					}

                					pid = stillActive[pidNum];

                				}
	
						/* fork/exec over to user.c to find out next action */
						cpid=fork();

						totalCount++;
						count++;
		
						if(cpid == 0) 
						{
							char passPid[10];
							sprintf(passPid, "%d", pid);		
							execl("./user","user", NULL);
							exit(0);
						}
				
						if (msgrcv(messageQ, &msg,sizeof(msg)+1,99,0) == -1) 
						{
								perror("msgrcv");

						}

						if(strcmp(msg.mtext, "WRITE") == 0)
                                                {
							ptr->resourceStruct.count+=1;
                                                        fprintf(fp,"Master: P%d Requesting write of address %d at %d:%d\n",pid,ptr->resourceStruct.request, ptr->time.seconds,ptr->time.nanoseconds);
                                                }

						if(strcmp(msg.mtext, "REQUEST") == 0)
                                                {
							ptr->resourceStruct.count+=1;
                                                        fprintf(fp,"Master: P%d Requesting read of address %d at %d:%d\n",pid,ptr->resourceStruct.request, ptr->time.seconds,ptr->time.nanoseconds);
						}

						/* if a process decides to terminate update stillActive array and release allocated resources */
						if(strcmp(msg.mtext, "TERMINATED") == 0)
						{
							fprintf(fp,"Master: Terminating P%d at %d:%d\n",pid, ptr->time.seconds,ptr->time.nanoseconds);
							stillActive[pid] = -1;
						}
			}
		}
	detach();
	return 0;
}
/* function to increment the clock and protect via semaphore */
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

void setUp()
{
	 /* setup shared memory segment */
        if ((shmid = shmget(9784, sizeof(sm), IPC_CREAT | 0600)) < 0)
        {
                perror("Error: shmget");
                exit(0);
        }

	if ( (messageQ = msgget(4020015, 0777 | IPC_CREAT)) == -1 ) 
	{
        	perror("OSS: Error generating message queue");
        	exit(0);
    	}	
	
        /* open semaphore to protect the clock */
        sem = sem_open("p5sem", O_CREAT, 0777, 1);

        /* attach shared memory */
        ptr = shmat(shmid, NULL, 0);

}

/* detach shared memory and semaphore */
void detach()
{
	shmctl(shmid, IPC_RMID, NULL);	
	msgctl(messageQ, IPC_RMID, NULL); 
	sem_unlink("p5sem");
	sem_close(sem);
}

/* Function to control two types of interupts */
void sigErrors(int signum)
{
        if (signum == SIGINT)
        {
		printf("\nInterupted by ctrl-c\n");
	}
        else
        {
                printf("\nInterupted by %d second alarm\n", timer);
	}
	
	detach();
        exit(0);
}
