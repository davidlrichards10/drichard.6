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

typedef struct shmStruct{
	resourceInfo resourceStruct;
	struct time time;
} sm;

#endif
