/* Name: David Richards
 * Date: Tue, May 5th
 * Assignment: hw6
 * file: oss.c
 */

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
#include "queue.h"

#define MAXPRO 18

/* for shared memory setup/semaphore */
int shmid; 
int shmid_mem;
sm* ptr;
sem_t *sem;

/* struct for message queues*/
struct message {
    long msgType;
    char mtext[512];
};

struct memory *mem; 
struct memory memstruct; 

struct message msg;

int faults;
int requests;
int pagenumber[MAXPRO][32];
int messageQ;
int stillActive[20];
int pidNum = 0;
int termed = 0;	
int secondsCount = 1;
int lines = 0;

void printStats();
void printMemLayout();
void setUp();
void detach();
void sigErrors();
void incClock(struct time* time, int sec, int ns);
int pageLocation(int, int);
void initPages();
int swapFrame();
int findFrame();
void pageSend(int, int, int);
void resetMemory(int);

int timer = 2;
int memoryAccess = 0;

/* default output file */
char outputFileName[] = "log";
FILE* fp;

int main(int argc, char* argv[]) 
{
	int c;
	int unblockNum = -1;
	int frameNumResult = 0;
	int findframe = 0;
    	int swapframe = -1;
	int sendNum = -1;
	int i = 0;
	
	/* getopt to parse command line options */
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

	/* Catch ctrl-c and 2 second alarm interupts */
	if (signal(SIGINT, sigErrors) == SIG_ERR) //sigerror on ctrl-c
        {
                exit(0);
        }

        if (signal(SIGALRM, sigErrors) == SIG_ERR) //sigerror on program exceeding specified second alarm
        {
                exit(0);
        }

	/* set up shared memory */
	setUp();

	/* create queue for blcoked processes */
	create();
	/* set up page numbers */
	initPages();

	/*initialize array for method 2 */
	float processWeight;
        for(i = 1; i <= 32; i++)
        {
               	processWeight = 1 / (float)i;
       		ptr->resourceStruct.weights[i - 1] = processWeight;
        }

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
			incClock(&ptr->time,0,10000);
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
							printStats();
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

						unblockNum = getProcess(); 
       	 					/* if the blocked queue is not empty grant request and remove from queue */
						if (unblockNum != -1) 
        					{
               	 					removeProcess(unblockNum);
						}
				
						if (msgrcv(messageQ, &msg,sizeof(msg)+1,99,0) == -1) 
						{
								perror("msgrcv");

						}

						/* if user communicates a write action */
						if(strcmp(msg.mtext, "WRITE") == 0)
                                                {
							/* get the address message */
							msgrcv(messageQ,&msg,sizeof(msg),99,0);	

							int write = atoi(msg.mtext);
							ptr->resourceStruct.count+=1;
							/* check if the address is available */
							if(ptr->resourceStruct.memType == 0)
							{
								frameNumResult = pageLocation(pid, write/1024);
							}
							if(ptr->resourceStruct.memType == 1)
                                                        {
                                                                frameNumResult = pageLocation(pid, (write/1024)/2);
                                                        }
							incClock(&ptr->time,0,14000000);
							fprintf(fp,"Master: P%d Requesting write of address %d at %d:%d\n",pid,write, ptr->time.seconds,ptr->time.nanoseconds);
							requests++;

							/* grant request if available */
							if (frameNumResult != -1) 
							{
								incClock(&ptr->time,0,10);
								mem->referenceBit[frameNumResult] = 1;
								mem->dirty[frameNumResult] = 1;
								incClock(&ptr->time,0,10000);
								fprintf(fp,"Address %d is in frame %d, writing data to frame at time %d:%d\n",write,frameNumResult,ptr->time.seconds,ptr->time.nanoseconds);
							}
							/* else a page fault occurs and a empty frame is filled or frame to replace is found */
							else
							{
								faults++;
								findframe = findFrame();
								fprintf(fp,"Address %d is not in a frame, pagefault\n", write);
								/* select a page to replace if memory is full */
								if (findframe == -1) 
								{
                    							swapframe = swapFrame();
									if(ptr->resourceStruct.memType == 0)
									{
										pageSend(pagenumber[pid][write/1024],swapframe, 0);
                    								mem->pagetable[pid][write/1024] = swapframe;
									}
									if(ptr->resourceStruct.memType == 1)
                                                                        {
                                                                                pageSend(pagenumber[pid][(write/1024)/2],swapframe, 0);
                                                                                mem->pagetable[pid][(write/1024)/2] = swapframe;
                                                                        }

									fprintf(fp,"Clearing frame %d and swapping in P%d\n",swapframe,pid);
									fprintf(fp,"Dirty bit of frame %d is set, adding time to clock\n",swapframe);
								}
								/* place the page in the next open frame */
                						else 
								{
									if(ptr->resourceStruct.memType == 0)
									{
                    								sendNum = pagenumber[pid][write/1024];
                    								pageSend(sendNum, findframe, 0);
                                                                        	mem->pagetable[pid][write/1024] = findframe;
									}
									if(ptr->resourceStruct.memType == 1)
                                                                        {
                                                                                sendNum = pagenumber[pid][(write/1024)/2];
                                                                                pageSend(sendNum, findframe, 0);
                                                                                mem->pagetable[pid][(write/1024)/2] = findframe;
                                                                        }
									fprintf(fp,"Clearing frame %d and swapping in P%d page %d\n",findframe,pid,sendNum);
									fprintf(fp,"Dirty bit of frame %d is set, adding time to clock\n",findframe);
								}
								/* add the process to the queue */
								addProcess(pid);	
							}
						}
						/* if user indicates a read request */
						if(strcmp(msg.mtext, "REQUEST") == 0)
                                                {
							/* get the address from user.c */
							msgrcv(messageQ,&msg,sizeof(msg),99,0);
                                                        int request = atoi(msg.mtext); 
							ptr->resourceStruct.count+=1;
                                                        /* check if the request can be granted */
							if(ptr->resourceStruct.memType == 0)
							{
							        frameNumResult = pageLocation(pid, request/1024);
							}
							if(ptr->resourceStruct.memType == 1)
                                                        {
                                                                frameNumResult = pageLocation(pid, (request/1024)/2);
                                                        }
							incClock(&ptr->time,0,14000000);
							fprintf(fp,"Master: P%d Requesting read of address %d at %d:%d\n",pid,request, ptr->time.seconds,ptr->time.nanoseconds);
							requests++;
							/* the read request can be granted */
							if (frameNumResult != -1)
                                                        {
								incClock(&ptr->time,0,10);
                                                                mem->referenceBit[frameNumResult] = 1;
                                                                fprintf(fp,"Address %d is in frame %d, giving data to P%d at time %d:%d\n",request,frameNumResult,pid,ptr->time.seconds,ptr->time.nanoseconds);
							}
							/* else a page fault occurs and a empty frame is filled or frame to replace is found */
                                                        else
                                                        {
								faults++;
                                                                findframe = findFrame();
                                                                fprintf(fp,"Address %d is not in a frame, pagefault\n", request);
                                                                /* if memory is full, select a page to replace */
								if (findframe == -1)
                                                                {
                                                                        swapframe = swapFrame();
                                                                     	if(ptr->resourceStruct.memType == 0)
									{
								          	pageSend(pagenumber[pid][request/1024],swapframe, 0);
                                                                                mem->pagetable[pid][request/1024] = swapframe;
									}
									if(ptr->resourceStruct.memType == 1)
                                                                        {
                                                                                pageSend(pagenumber[pid][(request/1024)/2],swapframe, 0);
                                                                                mem->pagetable[pid][(request/1024)/2] = swapframe;
                                                                        }

                                                                        fprintf(fp,"Clearing frame %d and swapping in P%d\n",swapframe,pid);
                                                                }
								/* else put the page in the next empty frame */
                                                                else
                                                                {
									if(ptr->resourceStruct.memType == 0)
									{
                                                                                sendNum = pagenumber[pid][request/1024];
                                                                                pageSend(sendNum, findframe, 0);
                                                                                mem->pagetable[pid][request/1024] = findframe;
									}
									if(ptr->resourceStruct.memType == 1)
                                                                        {
                                                                                sendNum = pagenumber[pid][(request/1024)/2];
                                                                                pageSend(sendNum, findframe, 0);
                                                                                mem->pagetable[pid][(request/1024)/2] = findframe;
                                                                        }

                                                                        fprintf(fp,"Clearing frame %d and swapping in P%d page %d\n",findframe,pid,sendNum);
                                                                }
								/* process is added to the queue */
								addProcess(pid);
                                                        }

						}

						/* if a process decides to terminate update stillActive array and release allocated resources */
						if(strcmp(msg.mtext, "TERMINATED") == 0)
						{
							fprintf(fp,"Master: Terminating P%d at %d:%d\n",pid, ptr->time.seconds,ptr->time.nanoseconds);
							stillActive[pid] = -1;
							resetMemory(pid);
						}
						/* print the memory layout every second */
						if(ptr->time.seconds == secondsCount)
						{
							secondsCount+=1;
							printMemLayout();
						}
						
						if(pidNum < 17)
						{			
							pidNum++;	
						} 
						else
						{
							pidNum = 0;
						}
			}
		}
	/* print stats and detach shared memory */
	printStats();
	detach();
	return 0;
}

/* function to print current memory layout */
void printMemLayout()
{
	fprintf(fp,"\nCurrent Memory layout at time %d:%d\n",ptr->time.seconds,ptr->time.nanoseconds);
	fprintf(fp,"\t     Occupied\tRef\t   DirtyBit\n");

	int i;
	for(i = 0; i < 256; i++)
	{
		fprintf(fp,"Frame %d:\t",i + 1);
		
		if (mem->bitvector[i] == 0)
		{
			fprintf(fp,"No\t");
		}
		else
		{
			fprintf(fp,"Yes\t");		
		}
		
		if (mem->bitvector[i] == 1)
		{
			if (mem->referenceBit[i] == 0)
			{
				fprintf(fp,"0\t");
			}
			else
			{
				fprintf(fp,"1\t");
			}
		}
		else
		{
			fprintf(fp,"0\t");
		}
		if (mem->dirty[i] == 0)
		{
			fprintf(fp,"0\t");
		}
		else
		{
			fprintf(fp,"1\t");
		}
		fprintf(fp,"\n");
	}

}

/* function to print relevant statistics */
void printStats()
{
	printf("\nMemory access  per second: %f\n",((float)(requests)/((float)(ptr->time.seconds))));	
	printf("Page faults per access: %f\n",((float)(faults)/(float)requests));
	printf("Average access time: %f\n\n", (((float)(ptr->time.seconds)+((float)ptr->time.nanoseconds/(float)(1000000000)))/((float)requests)));

	fprintf(fp,"\nMemory access  per second: %f\n",((float)(requests)/((float)(ptr->time.seconds))));
        fprintf(fp,"Page faults per access: %f\n",((float)(faults)/(float)requests));
        fprintf(fp,"Average access time: %f\n\n", (((float)(ptr->time.seconds)+((float)ptr->time.nanoseconds/(float)(1000000000)))/((float)requests)));
}

/* on termination, clear the frame and reset information */
void resetMemory(int id) 
{
    	int i; 
    	int frame;
    	int page;
    	for (i=0; i<32; i++) 
    	{
        	if(mem->pagetable[id][i] != -1) 
        	{
            		frame = mem->pagetable[id][i];
            		page = pagenumber[id][i];
            		mem->referenceBit[frame] = 0;
            		mem->dirty[frame] = 0;
            		mem->bitvector[frame] = 0;
            		mem->frame[frame] = -1;
            		mem->pagetable[id][i] = -1;
            		mem->pagelocation[page] = -1;
        	}
	}
}

/* return the next empty frame */
int findFrame() 
{
    	int i;
    	for (i=0; i<256; i++) 
	{
        	if (mem->bitvector[i] == 0) 
		{
            		return i;
        	}
    	}
        return -1;
}

/* set information about page */
void pageSend(int pageNum, int frameNum, int dirtybit) 
{
    	mem->referenceBit[frameNum] = 1;
    	mem->dirty[frameNum] = dirtybit;
    	mem->bitvector[frameNum] = 1;
    	mem->frame[frameNum] = pageNum;
    	mem->pagelocation[pageNum] = frameNum;
}

/* finds page to replace */
int swapFrame() 
{
    	int frameNum;
    	while(1) 
	{
		/* reset last chance bit */
        	if (mem->referenceBit[mem->referenceStat] == 1) 
		{
            		mem->referenceBit[mem->referenceStat] = 0;
            		if (mem->referenceStat == 255) 
			{
				mem->referenceStat = 0;
			}
			else 
			{
				mem->referenceStat++;
			}
        	}
		/* on last chance, so replace frame */
        	else 
		{
           		frameNum = mem->referenceStat;
            		if(mem->referenceStat == 255) 
			{
				mem->referenceStat = 0;
			}
            		else 
			{
				mem->referenceStat++;
			}
            		return frameNum;
        	}
    	}
}

/* function to return frame number if it is available otherwise return -1 */
int pageLocation(int u_pid, int u_num)
{
    	int i;
	int pageNum; 
	int frameNum;

    	pageNum = pagenumber[u_pid][u_num];

    	frameNum = mem->pagelocation[pageNum];

    	if (frameNum == -1) 
	{
       		return -1;
    	}

    	if (mem->frame[frameNum] == pageNum) 
	{
        	return frameNum;
    	}

    	return -1;
}

/* function that sets page numbers */
void initPages()
{
    	int process;
	int num;
	int i;
    	for (process=0; process<18; process++) 
	{
        	for (num=0; num<32; num++) 
		{
            		mem->pagetable[process][num] = -1;
            		pagenumber[process][num] = process*32 + num;
        	}
    	}
    	for (num = 0; num < 576; num++) 
	{
        	mem->pagelocation[num] = -1;
    	}
    	for (i=0; i<256; i++) 
	{
        	mem->frame[i] = -1;
    	}
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

/* set up shared memory */
void setUp()
{
	/* set up shared memory segment */
	shmid_mem = shmget(4020014, sizeof(memstruct), 0777 | IPC_CREAT);
    	if (shmid_mem == -1) 
	{
            exit(0);
        }
    	mem = (struct memory *) shmat(shmid_mem, NULL, 0);
    	if (mem == (struct memory *)(-1) ) 
	{
        	exit(0);
    	}

	 /* setup shared memory segment */
        if ((shmid = shmget(9784, sizeof(sm), IPC_CREAT | 0600)) < 0)
        {
                exit(0);
        }

	if ( (messageQ = msgget(4020015, 0777 | IPC_CREAT)) == -1 ) 
	{
        	perror("Error: message queue");
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
	shmctl(shmid_mem, IPC_RMID, NULL);
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
		detach();
		printStats();
	}
        else
        {
                printf("\nInterupted by %d second alarm\n", timer);
		detach();
		printStats();
	}
        exit(0);
}
