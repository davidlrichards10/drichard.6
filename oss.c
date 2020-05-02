
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

struct message {
    long msgType;
    char mtext[512];
};

struct memory *mem; 
struct memory memstruct; 

struct message msg;

int pagenumber[MAXPRO][32];
int messageQ;
int stillActive[20];
int pidNum = 0;
int termed = 0;	
int secondsCount = 1;
int lines = 0;

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

int timer = 5;
int memoryAccess = 0;

/* default output file */
char outputFileName[] = "log";
FILE* fp;

int main(int argc, char* argv[]) 
{
	/* getopt to parse command line options */
	int c;
	int frameNumResult = 0;
	int findframe = 0;
    	int swapframe = -1;
	int sendNum = -1;
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

	create();
	initPages();

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
							msgrcv(messageQ,&msg,sizeof(msg),99,0);	
							int write = atoi(msg.mtext);	
							ptr->resourceStruct.count+=1;
                                                        frameNumResult = pageLocation(pid, write);
							incClock(&ptr->time,0,14000000);
							fprintf(fp,"Master: P%d Requesting write of address %d at %d:%d\n",pid,write, ptr->time.seconds,ptr->time.nanoseconds);
							if (frameNumResult != -1) 
							{
								incClock(&ptr->time,0,10);
								mem->refbit[frameNumResult] = 1;
								mem->dirtystatus[frameNumResult] = 1;
								incClock(&ptr->time,0,10000);
								fprintf(fp,"Address %d is in frame %d, writing data to frame at time %d:%d\n",write,frameNumResult,ptr->time.seconds,ptr->time.nanoseconds);
							}
							else
							{
								findframe = findFrame();
								fprintf(fp,"Address %d is not in a frame, pagefault\n", write);
								if (findframe == -1) 
								{
                    							swapframe = swapFrame();
                    							pageSend(pagenumber[pid][write],swapframe, 0);
                    							mem->pagetable[pid][write] = swapframe;
                							fprintf(fp,"Clearing frame %d and swapping in %d\n",swapframe,pid);
								}
                						else 
								{
                    							sendNum = pagenumber[pid][write];
                    							pageSend(sendNum, findframe, 0);
                    							mem->pagetable[pid][write] = findframe;
                							fprintf(fp,"Clearing frame %d and swapping in %d\n",findframe,pid);
									fprintf(fp,"Dirty bit of frame %d is set, adding time to clock\n",findframe);
								}
								addProcess(pid);	
							}
						}
						if(strcmp(msg.mtext, "REQUEST") == 0)
                                                {
							msgrcv(messageQ,&msg,sizeof(msg),99,0);
                                                        int request = atoi(msg.mtext); 
							ptr->resourceStruct.count+=1;
							frameNumResult = pageLocation(pid, request);
                                                        incClock(&ptr->time,0,14000000);
							fprintf(fp,"Master: P%d Requesting read of address %d at %d:%d\n",pid,request, ptr->time.seconds,ptr->time.nanoseconds);
							
							if (frameNumResult != -1)
                                                        {
								incClock(&ptr->time,0,10);
                                                                mem->refbit[frameNumResult] = 1;
                                                                fprintf(fp,"Address %d is in frame %d, giving data to P%d at time %d:%d\n",request,frameNumResult,pid,ptr->time.seconds,ptr->time.nanoseconds);
                                                        }
                                                        else
                                                        {
                                                                findframe = findFrame();
                                                                fprintf(fp,"Address %d is not in a frame, pagefault\n", request);
                                                                if (findframe == -1)
                                                                {
                                                                        swapframe = swapFrame();
                                                                        pageSend(pagenumber[pid][request],swapframe, 0);
                                                                        mem->pagetable[pid][request] = swapframe;
                                                                        fprintf(fp,"Clearing frame %d and swapping in %d\n",swapframe,pid);
                                                                }
                                                                else
                                                                {
                                                                        sendNum = pagenumber[pid][request];
                                                                        pageSend(sendNum, findframe, 0);
                                                                        mem->pagetable[pid][request] = findframe;
                                                                        fprintf(fp,"Clearing frame %d and swapping in %d\n",findframe,pid);
                                                                }
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
	detach();
	return 0;
}

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
			if (mem->refbit[i] == 0)
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
			fprintf(fp,".\t");
		}
		if (mem->dirtystatus[i] == 0)
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
            		mem->refbit[frame] = 0;
            		mem->dirtystatus[frame] = 0;
            		mem->bitvector[frame] = 0;
            		mem->frame[frame] = -1;
            		mem->pagetable[id][i] = -1;
            		mem->pagelocation[page] = -1;
        	}
	}
}

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

void pageSend(int pageNum, int frameNum, int dirtybit) 
{
    	mem->refbit[frameNum] = 1;
    	mem->dirtystatus[frameNum] = dirtybit;
    	mem->bitvector[frameNum] = 1;
    	mem->frame[frameNum] = pageNum;
    	mem->pagelocation[pageNum] = frameNum;
}

int swapFrame() 
{
    	int frameNum;
    	while(1) 
	{

        	if (mem->refbit[mem->refptr] == 1) 
		{
            		mem->refbit[mem->refptr] = 0;
            		if (mem->refptr == 255) 
			{
				mem->refptr = 0;
			}
			else 
			{
				mem->refptr++;
			}
        	}
        	else 
		{
           		frameNum = mem->refptr;
            		if(mem->refptr == 255) 
			{
				mem->refptr = 0;
			}
            		else 
			{
				mem->refptr++;
			}
            		return frameNum;
        	}
    	}
}

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

void setUp()
{
	shmid_mem = shmget(4020014, sizeof(memstruct), 0777 | IPC_CREAT);
    	if (shmid_mem == -1) { //terminate if shmget failed
            perror("OSS: error in shmget for memory struct");
            exit(1);
        }
    	mem = (struct memory *) shmat(shmid_mem, NULL, 0);
    	if (mem == (struct memory *)(-1) ) {
        	perror("OSS: error in shmat liveState");
        	exit(1);
    	}

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
	}
        else
        {
                printf("\nInterupted by %d second alarm\n", timer);
		detach();
	}
	
	//detach();
        exit(0);
}
