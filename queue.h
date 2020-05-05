/* Name: David Richards
 * Date: Tue, May 5th
 * Assignment: hw6
 * file: queue.h
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>
#include "shared.h"

struct waitQueue {
    int queue[18]; 
};

struct waitQueue wPtr;

/* initialize the queue */
void create() 
{
    	int i;
    	for(i=0; i<18; i++) 
	{
        	wPtr.queue[i] = -1;
    	}
}

/* get next process from the queue */
int getProcess() 
{
    	if (wPtr.queue[0] == -1) 
	{
        	return -1;
    	}
    	else 
	{
		return wPtr.queue[0];
	}
}

/* insert a process into the queue if blocked */
int addProcess (int waitPid) 
{
    	int i;
    	for (i=0; i<18; i++) 
	{
        	if (wPtr.queue[i] == -1) 
		{
            		wPtr.queue[i] = waitPid;
            		return 1;
        	}
    	}
    	return -1;
}

/* remove a process from the queue if request is granted */
int removeProcess(int pNum) 
{
    	int i;
    	for (i=0; i<18; i++) 
	{
        	if (wPtr.queue[i] == pNum) 
		{ 
            		while(i+1 < 18) 
			{
                		wPtr.queue[i] = wPtr.queue[i+1];
                		i++;
            		}
            		wPtr.queue[17] = -1;
            		return 1;
        	}
    	}
    	return -1;
}

#endif
