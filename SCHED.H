/* Scheduler Structures and Definitions */

#define MIN_EVENT 4		/* minimum result to trigger an event */
#define MAX_EVENT 29		/* maximum */

#define YELL_EVENT 1		/* Yell command enable */

#define MINS_HR	60		/* minutes in an hour */
#define DAYS_WK 7		/* days in a week */
#define MINS_DAY (24 * 60)	/* minutes in an hour */
#define MINS_WK (MINS_DAY * DAYS_WK)

#define SCHEDS (5 * DAYS_WK)	/* size of time table */


/* Standard time of day structure. */

struct _time {
	int year,month,day,daywk;
	int hour,mins,sec;
};

/* Scheduler time structure. */

struct _sched {
	struct _time time;
	int len;
	int cond;		/* condition */
	int trigger;
	int result;		/* returned value */
	char tag;		/* schedule tag */
	int a,b,c,d,e;

} sched[SCHEDS];

char tag;		/* handy local copy of the tag */
int event;		/* handy local copy of the current event */


char _daynames[7][4] = {
	"Sun",
	"Mon",
	"Tue",
	"Wed",
	"Thu",
	"Fri",
	"Sat"
};
