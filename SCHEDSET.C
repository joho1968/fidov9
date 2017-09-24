#include <stdio.h>
#include <ctype.h>
#include <bbs.h>
#include <sched.h>

/* List all the scheduled events. */

list_sched() {

int i;

	for (i= 0; i < SCHEDS; i++) {
		if (sched[i].result > 0) {
			mprintf("#%-2u on %s ",i,_daynames[sched[i].time.daywk]);
			astwo(sched[i].time.hour);
			mconout(':');
			astwo(sched[i].time.mins);
			mprintf(", %u min.  Return Code %2u ",sched[i].len,sched[i].result);
			mprintf("Schedule %c\r\n",sched[i].tag);
		}
	}
}
/* Enter/change an event. Get all the parts, check them, then install it
all at once. */

new_sched() {

char buf[80],*p;
int event,daywk,hour,mins,len,result;
char tag;

/* Get the event number. */

	*buf= '\0';
	getfield(buf,"Event # [1 - 19]: ",0,1,sizeof(buf),1);
	if (*buf == 'Y') event= 0;
	else if (isdigit(*buf)) event= atoi(buf);
	else return;

	if (event >= SCHEDS) {
		mprintf("Must be Event #1 to %u or Yell\r\n",SCHEDS - 1);
		return;
	}

/* Get the day, since Im lazy, entered as the number of the day of the
week. */

	getfield(buf,"Day of week [0=Sun, 1=Mon, ...]: ",0,1,sizeof(buf),1);
	if (*buf == '\0') return;
	daywk= atoi(buf);
	if (daywk > 6) {
		mprintf("Must be 0 - 6\r\n");
		return;
	}

/* Enter the time, in either HH:MM or HH MM format. Skipping minutes is OK;
it then defaults to HH:00. */

	getfield(buf,"Start Time [hh:mm]: ",0,2,sizeof(buf),1);
	if (*buf == '\0') return;
	p= buf;
	hour= atoi(p);
	while (*p) {
		if ((*p == ':') || (*p == ' ')) {
			++p;
			break;
		}
		++p;
	}
	mins= atoi(p);
	if ((hour > 23) || (mins > 59)) {
		mprintf("Must be a normal time; 00:00 to 23:59\r\n");
		return;
	}

/* Enter the window width and result code. */

	getfield(buf,"Window Width, Minutes: ",0,1,sizeof(buf),1);
	if (*buf == '\0') return;
	len= atoi(buf);
	if (len > MINS_DAY) {
		mprintf("There are only %u minutes in a day!\r\n",MINS_DAY);
		return;
	}

	getfield(buf,"Return Code [0 - 32767]: ",0,1,sizeof(buf),1);
	if (*buf == '\0') return;
	result= atoi(buf);

	getfield(buf,"Schedule Tag: ",1,1,1,1);
	tag= *buf;

	sched[event].time.year= 0;
	sched[event].time.month= 0;
	sched[event].time.day= 0;
	sched[event].time.daywk= daywk;
	sched[event].time.hour= hour;
	sched[event].time.mins= mins;
	sched[event].time.sec= 0;
	sched[event].len= len;
	sched[event].result= result;
	sched[event].tag= tag;
	sched[event].cond= 0;
	sched[event].trigger= 0;

	put_sched();
}
