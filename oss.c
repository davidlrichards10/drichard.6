/* Author: David Richards
 * Date: Thurs April 23rd
 * Assignment: CS4760 hw6
 * File: oss.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include "shared.h"

int shmid;
sm* ptr;

void setUp();
void detach();
void sigErrors();

int timer = 2;
int memoryAccess = 0;
char outputFileName[] = "log";
FILE* fp;

int main(int argc, char* argv[]) 
{

	/* getopt to parse command line options */
	int c;
	while((c=getopt(argc, argv, "m:i:t:h"))!= EOF)
	{
		switch(c)
		{
			case 'h':
				printf("\nInvocation: oss [-h] [-i x -m x -t x]\n");
                		printf("----------------------------------------------Program Options-------------------------------------------\n");
                		printf("       -h             Describe how the project should be run and then, terminate\n");
				printf("       -i             Type file name to print program information in (Default of log)\n");
				printf("       -m             Indicate memory access type by typing 0 or 1 (Default 0)\n");
				printf("       -t             Indicate timer amount (Default of 10 seconds)\n");				
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

	/* Catch ctrl-c and alarm interupts */
	if (signal(SIGINT, sigErrors) == SIG_ERR) //sigerror on ctrl-c
        {
                exit(0);
        }

        if (signal(SIGALRM, sigErrors) == SIG_ERR) //sigerror on program exceeding specified second alarm
        {
                exit(0);
        }

	alarm(timer);
	
	setUp();
	detach();

	return 0;
}

/* set up shared memory */
void setUp()
{
	/* setup shared memory segment */
	if ((shmid = shmget(9784, sizeof(sm), IPC_CREAT | 0600)) < 0) 
	{
        	perror("Error: shmget");
        	exit(0);
	}
	
	ptr = shmat(shmid, NULL, 0);
	

}

/* detach shared memory */
void detach()
{
	shmctl(shmid, IPC_RMID, NULL);	
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
