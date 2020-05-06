/* Author: David Richards
 * Date: Thurs April 23rd
 * Assignment: CS4760 hw6
 * File: shared.h
 */

#ifndef SHARED_H
#define SHARED_H

#define MAXPRO 18

struct time{
	int nanoseconds;
	int seconds;
};

typedef struct {
	int memType;
	int count;
	int write;
	float weights[32];
} resourceInfo;

struct memory {
    	int referenceBit[256]; 
    	int dirty[256]; 
    	int bitvector[256]; 
    	int frame[256]; 
    	int referenceStat; 
    	int pagetable[MAXPRO][32]; 
    	int pagelocation[576]; 
};

typedef struct shmStruct{
	resourceInfo resourceStruct;
	struct time time;
} sm;

#endif
