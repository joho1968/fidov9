#include <stdio.h>
#include <ctype.h>
#include <bbs.h>
#include <mail.h>
#include <a:/lib/ascii.h>

/* Main function for Fido. Handles one caller, then returns to the
main section. */

do_fido() {

int time,i;
int ev,mins_til_mail;

	cmtfile= _xopen("sysop.log",2);
	if (cmtfile == -1) {
		cmtfile= _xcreate("sysop.log",2);
		if (cmtfile == -1) {
			cprintf("ERROR: cmtfile create error.\r\n");
			doscode= 1;
			return;
		}
	} else {
		_xseek(cmtfile,0L,2);	/* seek to end; append */
	}

	echo= 1;				/* set defaults for name entry, */
	user-> tabs= 0;				/* dont expand tabs, */
	user-> nulls= 0;			/* send some nulls, */
	user-> more= 0;
	user-> width= 80;			/* default screen width */
	user-> len= 24;				/* and length, */
	*user-> name= '\0';
	for (i= 0; i < sizeof(mail_checked); i++)
		mail_checked[i]= 0;		/* clear check mail stuff */

	abort= 0;
	column= 0;
	line= 0;

	userflag= 0;					/* dont write in the */
	cmtflag= 0;					/* user file or cmt file */
	limit= 0;					/* stop time limit, */
	clr_clk();					/* no elapsed time */
	typemode= 0;
	system(0,0);					/* initial system file */

	set_abort(0);				/* set carrier loss trap */
	if (was_abort()) {			/* return to main */
		_xclose(cmtfile);
		if (!test) discon();		/* drop carrier */
		return;
	}

	if (! test) {

		if ((mdmtype == SMARTCAT) && !test) init_modem("fidomdm.bbs");

/* Carrier detected. Start counting minutes now. Initially, the limit is set
to 5 minutes, to let the guy hit CR, enter his name, etc. Signon() then
sets the limit to the correct one. */

		while (mconstat()) mconin();		/* flush input, */

		cprintf("(2)       Carrier detected.\r\n");
		limit= slimit;				/* allow 5 min */
		clr_clk();				/* start counting */
		wlimit= 1;				/* no warning msg */
		cprintf("(3)       Determining baud rate.\r\n");
		connect();				/* get baud, etc. */
		cprintf("(4)       Connected at %s baud.\r\n",
			((rate == 0) ? "300" : "1200") );
	}
	wlimit= 0;				/* enable limit warning */

/* Physically connected. Do some housekeeping, then get the users name,
password, etc. */

	gtod(date);			/* get signon date, */
	modout(CR); modout(CR);		/* CRs for FidoNet dialer */
	mprintf("Fido BBS (c) tj v%s\r\n",VER);
	if (mail.node) mprintf("FidoNet Node #%u\r\n",mail.node);

	if (signon() == 0)		/* signon the user, */
		logoff(0);		/* log him off if error. */

	pathnum= user-> files;		/* set default paths, */
	msgnum= user-> msg;		/* from user record */

/* Make sure the system file numbers are valid: files might have been
deleted, etc or previous version. (v5 and less used user-> msg as the
last read message number.) */

	if ((pathnum == 0) || (pathnum > sysfiles)) pathnum= 1;
	if ((msgnum == 0) || (msgnum > sysfiles)) msgnum= 1;

/* Adjust the time limits, etc for the type of user we have. */

	switch(user-> priv) {
		case TWIT:
			limit/= 2;	/* 1/2 time limit, */
			klimit/= 2;	/* 1/2 download limit, */
			dlimit/= 2;	/* 1/2 daily limit, */
			break;
		case DISGRACE:
		case NORMAL:
			break;
		case PRIVEL:
		case EXTRA:
			limit*= 2;
			klimit*= 2;
			dlimit*= 2;
			break;
		case SYSOP:
			limit= 0;	/* no limit */
			dlimit= 0;
			klimit= 0;
			break;
		default:
			lprintf("BAD PRIVELEGE= %u\r\n",user-> priv);
			user-> priv= NORMAL;
			break;
	}
	userflag= 1;			/* mark files as OK */
	cmtflag= 1;			/* to update at logoff */

/* Put some signon info into the logfile. */

	lprintf("---------------------------\r\n");
	lprintf("%s on at %s.\r\n",user-> name,date);

	++user-> times;			/* he called again, */

	timelog();			/* keep statistics, */

	if (days(user-> ldate,date) > 1) {/* clear limits if more */
		user-> time= 0;		/* than one day, */
		user-> dnldl= 0;
	} 
/* .........................................
Fix a bug in v8d-a */

	switch (user-> help) {
		case NOVICE:
		case REGULAR:
		case EXPERT:
			break;
		default:
			user-> help= NOVICE;
			break;
	}

/* If we are doing mail, and near the mail time, limit the callers time on
to avoid hanging up Fidonet. */

	mins_til_mail= til_sched();		/* time til next event */

	if ((mins_til_mail > 0) && (mins_til_mail < 3))
		mins_til_mail= 2;		/* be reasonable */

	if ( (mins_til_mail > 0)
	    && ((mins_til_mail < limit) || (limit == 0)) ) {

		limit= mins_til_mail;
		mprintf("\r\n\r\n\r\nNOTE:\r\n");
		mprintf("   It is only %u minutes until FidoNet mail time.\r\n",limit);
		mconflush();
		if (!verify("Want to continue")) goodbye("");
	}

	abort= 0;
	bbsmsg("welcome2");		/* display welcome message, */
	abort= 0;
	bbsmsg("bulletin");		/* display bulletins, */
	abort= 0;
	line= 0;			/* syncup "more" stuff, */
	mprintf("You are the "); nth(sys-> caller);  mprintf(" caller.\r\n");

	quote("quotes.bbs");		/* quote for the day, */

	mprintf("Wait ...\r\n");
	system(0,msgnum);		/* force default system file, */
	if (sys-> priv > user-> priv) {	/* dont allow access to files */
		msgnum= 1;		/* with high priveleges */
		system(0,msgnum);	/* SYSTEM1.BBS always accessible. */
	}
	msgcount();			/* count messages */

/* Check the global limits, see if we can let him go. */

	if ((dlimit > 0) && (user-> time > dlimit)) {
		mprintf("You have exceeded the daily maximum time limit.\r\n");
		lprintf("Time: forced logoff\r\n");

		goodbye("");
	}
	main_menu();
}
