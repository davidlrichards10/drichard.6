
#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>
#include "shared.h"

struct waitQueue {
    int queue[18]; 
    unsigned int waitSec[18];
    unsigned int waitNS[18];
};

struct waitQueue wPtr;

void create() 
{
    	int i;
    	for(i=0; i<18; i++) 
	{
        	wPtr.queue[i] = -1;
    	}
}

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
