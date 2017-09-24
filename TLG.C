#include <bbs.h>

/* Update (or create) the time of day log. Increment the counter for the
particular hour and day. */

timelog() {
int daywk,hour,i;
int timefile;
char newname[20];
struct _tlog tlg;

	timefile= _xopen("timelog.bbs",2);		/* if not there, create, */
	if (timefile == -1) {
		timefile= _xcreate("timelog.bbs",2);
		if (timefile == -1) {
			cprintf("Cant create TIMELOG.BBS\r\n");
			return;
		}
		gtod(tlg.fdate);
		gtod(tlg.ldate);
		tlg.calls= 0;
		for (daywk= 0; daywk < 7; daywk++) {
			for (hour= 0; hour < 24; hour++)
				tlg.log[daywk][hour]= 0;
		}
	} else {
		_xread(timefile,&tlg,sizeof(tlg));	/* else read it in, */
		if (days(tlg.fdate,date) > 6) {		/* if a week old, */
			_xclose(timefile);		/* close it up, */
			gtod(newname);			/* rename to todays */
			newname[9]= '\0';		/* date, */
			strcpy(&newname[2],&newname[3]);/* remove spaces */
			strcpy(&newname[5],&newname[6]);
			strcat(newname,".tlg");
			_xrename("timelog.bbs",newname);/* rename it */
			_xdelete("timelog.bbs");	/* last chance stop recursion */
			timelog();			/* create new one */
			return;
		}
	}
	hour= gtod3(4); if (hour > 23) hour= 0;		/* get time (error */
	daywk= gtod3(3); if (daywk > 6) daywk= 0;	/* check & bound) */

	++tlg.log[daywk][hour];				/* increment */
	++tlg.calls;
	strcpy(tlg.ldate,date);				/* last access, */
	_xseek(timefile,0L,0);
	_xwrite(timefile,&tlg,sizeof(tlg));		/* write it out, */
	_xclose(timefile);
}
