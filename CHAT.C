#include <stdio.h>
#include <ctype.h>
#include <bbs.h>
#include <sched.h>

extern int seconds,minutes,hours;

/* Chat mode code. */


/* See if the sysop's available, if so, beep and get his attention. Sysop must
type the right special character to acknowledge. */

chat() {

int i;
char c;

	if (is_sched('Y') == -1) {
		mprintf("Yelling at the Sysop isn't allowed right now.\r\n");
		return;
	}
	if (test) {
		mprintf("Wont work in test mode!\r\n");
		return;
	}

	mprintf("Yelling for the sysop, hold on ...");
	cprintf("\r\n***************** Yelling for sysop ***************\r\n");
	cprintf("  User: %s Times: %u Mins. On: %u  c == chat, space == unavailable\r\n",
		user-> name,user-> times,minutes);

	for (i= 10; i; i--) {
		if (!ding) bdos(6,'\07'); /* ding once if bell not allowed, */
		mconout('\07'); delay(25); mconout('\07');
		delay(100);
		if (c= keyhit()) {
			c= tolower(c);
			if (c == 'c') {
				talk();
				break;

			} else if (c == ' ') {
				i= 0;
				break;
			}
		}
	}
	if (i == 0) {
		mprintf("\r\nSysop is not available.\r\n");
		lprintf("Yell, sysop not available.\r\n");
	}
}

/* Talk back and forth. Exit when sysop types ^Z. */

talk() {

char c;
int moresave;
int typesave;

	mprintf("\r\n---------- Yell at the Sysop ----------\r\n");
	cprintf("Sysop: Control-Z to terminate.\r\n");

	moresave= user-> more; user-> more= 0;		/* dont need more here, */
	typesave= typemode; typemode= 1;		/* enable local input */

	while (typemode) {				/* 0 if local console ^Z */
		if (mconstat()) {			/* user echo to console, */
			c= mconin() & 0x7f;		/* get it, */
			if (c == 0x7f) c= 8;
			if (c == 0x1a) break;		/* terminate on ^Z */
			mconout(c);			/* echo to user + console, */
			if (c == '\r') mconout('\n');	/* auto LF, */
		}
	}
	line= 0;
	mprintf("\r\n---------- Done Yelling at the Sysop ----------\r\n");
	user-> more= moresave;
	typemode= typesave;
}
