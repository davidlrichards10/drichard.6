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
} resourceInfo;

struct memory {
    int refbit[256]; 
    int dirtystatus[256]; 
    int bitvector[256]; 
    int frame[256]; 
    int refptr; 
    int pagetable[MAXPRO][32]; 
    int pagelocation[576]; 
};

typedef struct shmStruct{
	resourceInfo resourceStruct;
	struct time time;
} sm;

#endif
