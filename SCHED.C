#include <stdio.h>
#include <ctype.h>
#include <bbs.h>
#include <sched.h>

/* Scheduler for Fido, etc. Various scheduling and support routines.
These use the MSDOS time and date, and assumes a continuous seven
day schedule. Resolution is one minute.

The scheduler is a structure array containing times, a window width,
result codes, etc. is_sched(c) searches the time table for an event tag that
matches the passed character; special wildcards are used. It returns the
index of an event that is within the time window, ie. ready to 
run. Event times are specified as a starting time and a width, in 
minutes. The triggers and conditionals aer not used yet.

Event 0 is reserved for the Fido Yell command enable.

For Fido v9 anyways, the only other use is for terminating Fido and
running Fidonet. The result code is the actual ERRORLEVEL value
passed to DOS; therefore, it may be used to trigger other external
programs by changing the result from 4 to something detectable in the
RUNBBS.BAT file.

The TAG tells what kind of event this is. Tags A - W are events that
cause Fido to terminate to DOS with the specified result code. Y is for
the Yell command enable. ? is a wildcard, and must not be used.

The following results are reserved:

0	Disable this event
1	reserved
2	Reserved
3	Reserved
4	FidoNet National
5 - 30	FidoNet Local 1 to 25
31 - 99	Reserved

1, 2, and 3 are Fido error returns; these cannot be triggered from
here. 4 is the standard FidoNet mail time window. 5 to 30 are the
special local FidoNet times; they control which node list is used.
31 to 99 are reserved. 100 - 255 may be used for other events.

is_sched(c)	Returns the index of the scheduled event that is
char c;		erady to run and matches thr specified tag. Returns
		-1 if nothing ready to run.

in_sched(n)	Returns the result code if the specified event is ready
int n;		to run, or 0 if not ready to run. This will return the
		result during the entire event window.

til_sched()	Returns the number of minutes til the next scheduled
		event. Returns 0 if no event is ready or scheduled 
within 32767 minutes.


list_sched()	List all the events.

new_sched()	Enter or change an event. Fill in the blanks.

get_sched()	Loads the time table from disk. If not there, it
		fills in one event for each day, enabling the Yell
command for 24 hrs each day.

put_sched()	Writes the time table out to disk.

*/

long lt();

/* Return the index of the event that is ready to run. ? matches
any tag. A result of 0 effectively disables that event. Returns -1 if
no event scheduled. */

is_sched(m)
char m;
{
int i,x;

	for (i= 0; i < SCHEDS; i++) {
		if ( (m == '?') ||
		  ((m == '#') && (sched[i].tag >= 'A') && (sched[i].tag <= 'W')) ||
		  (m == sched[i].tag) ) {
			x= in_sched(i);
			if (x > 0) return(i);
		}
	}
	return(-1);
}

/* Return the result code of this event if it is ready to be run. */

in_sched(n)
int n;
{
long now,start,end;
struct _time t;

	if ((sched[n].result > 0) && (n < SCHEDS)) {
		get_time(&t);			/* get current time, */
		now= lt(&t);			/* long time */
		start= lt(&sched[n].time);
		end= start + sched[n].len;
		if ((now >= start) && (now < end)) return(sched[n].result);
	}
	return(0);
}
/* Return the number of minutes until the START of the next scheduled
event. Returns 0 if no event scheduled, or if more than 32767 minutes.
NOTE: Returns the time until the NEXT event, even if in the middle of
another. */

til_sched() {

int i;
struct _time t;
long now,start,til;

	get_time(&t);
	now= lt(&t);

	for (i= 0; i < SCHEDS; i++) {
		if (sched[i].result >= MIN_EVENT) {
			start= lt(&sched[i].time);
			if ((now < start) && ((start - now) < 32767L))
				return((int) (start - now));
		}
	}
	return(0);
}

/* Convert the time structure elements dayinweek, hour minute to a 
long integer minutes since Sun. 00:00 */

long lt(t)
struct _time *t;
{
	return((t-> daywk * MINS_DAY) + (t-> hour * MINS_HR) + t-> mins);
}

/* Get the current time. Do it repeatedly until it stops changing. (This
of course assumes that the get time routine takes less than one second.) */

get_time(t)
struct _time *t;
{
struct _time x;
int i;

	i= 10;
	do {
		x.year= gtod3(0);
		x.month= gtod3(1);
		x.day= gtod3(2);
		x.daywk= gtod3(3);
		x.hour= gtod3(4);
		x.mins= gtod3(5);
		x.sec= gtod3(6);

		t-> year= gtod3(0);
		t-> month= gtod3(1);
		t-> day= gtod3(2);
		t-> daywk= gtod3(3);
		t-> hour= gtod3(4);
		t-> mins= gtod3(5);
		t-> sec= gtod3(6);
	} while ((x.sec != t-> sec) && --i);
	if (i== 0) {
		cprintf("SCHEDULER ERROR: Cant get the time of day!\r\n");
		logoff(1);
	}
}
/* Read the time table into the array. If error, fill it with the
defaults. */

get_sched() {

int f;

	f= _xopen("sched.bbs",0);
	if (f == -1) {				/* if doesnt exist, */
		for (f= 0; f < SCHEDS; f++) {	/* clear it all out */
			sched[f].time.year= 0;
			sched[f].time.month= 0;
			sched[f].time.day= 0;
			sched[f].time.daywk= 0;
			sched[f].time.hour= 0;
			sched[f].time.mins= 0;
			sched[f].time.sec= 0;
			sched[f].len= 0;
			sched[f].result= 0;
			sched[f].cond= 0;
			sched[f].trigger= 0;
			sched[f].tag= '-';
		}
		for (f= 0; f < DAYS_WK; f++) {
			sched[f].time.daywk= f;
			sched[f].time.hour= 10;	/* Yell OK between */
			sched[f].len= 840;	/* 10:00 AM - Midnite */
			sched[f].result= 1;
			sched[f].tag= 'Y';
		}
		return;
	}

	_xread(f,sched,sizeof(sched));
	_xclose(f);
}

/* Write the schedule out to disk. */

put_sched() {

int f;

	f= _xcreate("sched.bbs",1);
	_xwrite(f,sched,sizeof(sched));
	_xclose(f);
}
