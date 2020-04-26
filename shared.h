/* Author: David Richards
 * Date: Thurs April 23rd
 * Assignment: CS4760 hw6
 * File: shared.h
 */

#ifndef SHARED_H
#define SHARED_H

struct time{
	int nanoseconds;
	int seconds;
};

typedef struct {
	int memType;
	int count;
	int write;
} resourceInfo;

struct processInfo{
	int pid;
	struct time unblockTime;
	int unblock;
	int frame;
};

typedef struct shmStruct{
	resourceInfo resourceStruct;
	struct time time;
	struct processInfo processes[18];
} sm;

#endif
