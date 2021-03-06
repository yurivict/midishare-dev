/*
  MidiShare Project
  Copyright (C) Grame 1999-2002

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Grame Research Laboratory, 11, cours de Verdun Gensoul 69002 Lyon - France
  research@grame.fr

*/


#include <stdio.h>
#include <time.h>
#include "pthread.h"
#include <unistd.h> 

#ifdef FFSEM
#include "lffifo-sem.h"
#define fifo_end(ff)	0
#define LIMIT	50000
#else
#include "lffifo.h"
#define fifo_end(ff)	(fifocell *)ff
#define LIMIT	250000
#endif

#define MAXTHREADS	10
#define MAXCOUNT 6*MAXTHREADS

typedef struct pont {
	long 	limit;
	long	stopped;
	long	duration;
} pont;

#define pile fifo
#ifndef POP
	typedef fifocell * testcell;
#	define POP(f,c) c=fifoget(f)
#	define CHECK(f,c,msg) POP(f,c); if(c==0) printf (msg)
#endif
#define PUSH fifoput
#define LASTCELL(c)		((c) == fifo_end(&gstack))

//#define cell fifocell;

/*
 *	Global variables
 */
pile	gstack;
fifocell	tbl[MAXCOUNT];
#ifdef FIFOV1
fifocell	dummy;
#define fifoinit(f)		fifoinit(f, &dummy);
#endif

/*
static void fifoprint(pile *f)
{
	printf ("fifo     : %p\n", f);
	printf ("fifo head: %p\n", f->head);
	printf ("fifo tail: %p\n", f->tail);
}
*/

// init stack with cells
void initstack (pile *f)
{
	int i;
	fifoinit (f);
	for(i=0; i < MAXCOUNT; i++) {
		PUSH (f, &tbl[i]);
	}
}

long stacktest (long n)
{
	testcell temp[6];
	int i; clock_t t;
	
	t = clock();
	while (n--) {
		for (i=0; i<6; i++) {
			CHECK(&gstack, temp[i], "error: pop returned null\n");
		}
		for (i=0; i<6; i++) PUSH(&gstack, temp[i]);
	}
	return clock() - t;
}

void* stacktestthread(void* pp)
{
	pont* p = (pont*) pp;
	p->duration = stacktest(p->limit);
	p->stopped = 1;
	return 0;
}


//-----------------------------------------------------------------
#ifndef COUNT
static long count (pile * ff)
{
	long count = 0;
	fifocell * cl;
#ifdef FFSEM
	cl = ff->head;
#else
	cl = ff->head->link;
#endif
	while (cl != fifo_end(ff)) {
		count++;
		cl = cl->link;
		if (count > MAXCOUNT)
			return -1;
	}
	return count;
}
#endif

void checkstack (pile * f, long n)
{
	long i = count(f);
	if (i != n) { 
		printf("error checking stack %p : count=%ld, (should be %ld)\n", f, i, n);
		printf("       fifo size: %ld\n", fifosize(f));
	}
}

void bench (int max)
{
	pthread_t	fils[MAXTHREADS];
	pont		bridge[MAXTHREADS]; 
	long		i, end, th;
	double		perf=0; struct sched_param sp;

	sp.sched_priority=63;
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
	
	for (th=1; th <= max; th++) {
		initstack (&gstack);

		printf("threads count:\t %ld \t", th); fflush (stdout);
		for (i=0; i<th; i++) {
		  	bridge[i].limit = LIMIT;
		  	bridge[i].stopped = 0; 
		  	bridge[i].duration = 0; 
		  	pthread_create(&fils[i], NULL, stacktestthread, &bridge[i]);
			sp.sched_priority=62;
			pthread_setschedparam(fils[i], SCHED_RR, &sp);
	  	}
	  	do {
			sleep(1);
		  	end = 1; 
			for (i=0; i<th; i++) end &= bridge[i].stopped;
	  	} while ( end == 0 );
#ifdef PROCESSCLOCK
		perf = bridge[0].duration; // 0;
#else
		for (i=0; i<th; i++) { perf += bridge[i].duration; }
#endif		
		perf /= th * LIMIT * 6;
		perf *= 1000000 / CLOCKS_PER_SEC;
		printf (" perf (in us per pop/push):\t %2f\n", perf); fflush (stdout);
		checkstack(&gstack, MAXCOUNT);
  	}
}


#define THREADSCOUNT	8

int main()
{
	printf("\n-------- Lock free fifo stack bench ----------\n");
	bench (THREADSCOUNT);
	return 0;
}
