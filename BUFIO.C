#include <stdio.h>
#include <ctype.h>
#include <bbs.h>
#include <a:/lib/ascii.h>

/*
	Console I/O for BBS


getstring(buff)	Input a string from the console, full editing.

mconout(c)	Output C to the console. It 'tabs' is true, expand with
		spaces to every 8 columns. If 'nulls' is set, send 'nulls'
		nulls after each linefeed. Echoes control characters as ^X.

mconin()	Returns a character from the console. 

mconstat()	Return true if a character available.

mprintf()	Same as PRINTF, except goes to the modem. 

modin(n)	Returns a character available, else -1 if more that N
		times 10 milliseconds.

modout(c)	Outputs C to the modem.

modstat()	Returns true if a modem character available.

dsr()		Returns true if Carrier Detect is true or CD_FLAG
		is true.

	The above all watch the modem via dsr(), and if it goes false,
logs the user off by calling logoff(0). (Closes the file, exits to DOS.)

*/

/* Send a character to the mode, expanding control characters. */

mconout(c)
char c;
{
	mcol(c,1);
}
/* Character to modem, control characters as is. */

mconoutr(c)
char c;
{
	mcol(c,0);
}
/* Send a character to the modem, processing control 
characters as indicated. */

mcol(c,f)
char c;
int f;
{
int i;

	switch (c) {

	case SUB:
		break;

	case LF:
		mconoutp(c);
		for (i= 0; i < user-> nulls; i++)
			mconoutp('\0');
		column= 0;
		++line;
		break;

	case TAB:
		if (user-> tabs) {
			do mcol(' ');
			while (column % 8);

		} else {
			mconoutp(c);
			column+= ((column + 7) % 8);
		}
		break;

	case CR:
	case CR + 128:
		mconoutp(CR);
		column= 0;
		break;

	case BEL:
		mconoutp(c);
		break;

	case BS:
		if (column) {
			mconoutp(c);
			--column;
		}
		break;
	default:
		if (c >= ' ') {
			mconoutp(c);
			++column;

		} else if (f) {
			mcol('^'); 
			mcol(c + '@');

		} else mconoutp(c);
		break;
	}
}
/* Output a character to the console, and poll for ^C, etc. Send all output
to the logfile. */

mconoutp(c)
char c;
{
char k;

	if ((user-> len > 10) && (line > user-> len)) {	/* if end of screen */
		line= 0;				/* reset to zero, */
		if (user-> more) more();
	}
	do {
		k= mconstat();
		if ((k == ETX) || (k == VT)) {		/* if ^C, or ^K, */
			mconflush();			/* flush it, */
			abort= 1;			/* set abort, */
			mconout(k);
		} else if (k == XOF) {			/* check ^S, */
			do {	k= mconin();
				if ((k == ETX) || (k == VT)) {
					abort= 1;
					mconout(k);
				}
			} while (k == XOF);
		}
	} while (!mconostat());

	if (! test)
		_mconout(c);			/* one to the modem, */
	if (c) {				/* if not null, */
		if (c == '\07') {		/* bells only if allowed, */
			if (ding) bdos(6,c);
		} else bdos(6,c);		/* one to the local console, */
		if (c != 0x1a) 			/* unless Control-Z, */
			log(c);			/* one to the log file. */
	}
}
/* Do the "More?" thing, reset line to zero. Must ignore ^S and ^Q, as it 
will get things all out of sync. */

more() {

char k;
int flag;

	line= 0;			/* reset, prevent infinite recursion, */
	mprintf("More ?");
	do {
		flag= 0;		/* only once, unless ^S, etc */
		mconflush();		/* flush typeahead, */
		k= mconin();
		switch(tolower(k)) {
			case 3:		/* Control-C */
			case 11:	/* control-K */
			case 'n':
				abort= 1;
				break;
			case 17:	/* Control-Q */
			case 19:	/* Control-S */
				flag= 1;
				break;
		}
	} while (flag);
	mconout(k);
	mprintf("\r\n");
}
/* Return 0 if no key available, else the key. */

mconstat() {

int t;
	
	limitchk();				/* check time limits, */
	pollkbd();				/* poll for chars, */
	t= (tail + 1) % sizeof(conbuf);		/* wrap pointer, */
	if (t == head)				/* if empty, */
		return(0);			/* return 0, */
	return( (int) conbuf[t]);		/* else the char, */
}
/* Wait for a character from the keyboard. */

mconin() {

	while (!mconstat());
	tail= ++tail % sizeof(conbuf);
	return( (int) conbuf[tail]);
}

/* Poll both keyboards, stuff characters into the ring buffer. This handles
all background tasks, like type mode, etc. */

pollkbd() {

char c;

	if (c= bdos(6,0xff)) {
		if (!test) {
			if (typemode) {
				if (c == 0x1a) {
					cprintf("End Simultaneous Keyboards\r\n");
					typemode= 0;
					c= '\0';
				}
			} else {
				if (c == 0x01) {
					cprintf("Simultaneous Keyboards\r\n");
					typemode= 1;
					c= '\0';	/* flush it, */

				} else if (tolower(c) == 'c') {
					talk();
					c= '\0';

				} else if (c == '?') {
					cprintf("\r\n----------------------------\r\n");
					cprintf(" %s Times: %u Mins. On: %u\r\n",
						user-> name,user-> times,minutes);
					cprintf("C  ... Chat    Z  ... Clear Limits\r\n");
					cprintf("^A ... Type On ^Z ... Type Off\r\n");
					cprintf("----------------------------\r\n");
					c= '\0';

				} else if (tolower(c) == 'z') {
					clr_clk();
					user-> dnldl= 0;
					cprintf("Time and Download Limits Cleared\r\n");
					c= '\0';
				}
			}
		}
	}
	if ((typemode || test) && (c != '\0')) {
		if ((c == ETX) || (c == VT) || (c == XOF)) 
			mconflush();
		if (head != tail) {
			conbuf[head]= c;
			head= ++head % sizeof(conbuf);
		}
	} 
	if (! test) {
		if (! dsr()) logoff(0);	/* dont get caught, */

		if (_mconstat()) {		/* if character there, */
			c= _mconin() & 0x7f;	/* get it, */
			if ((c == ETX) || (c == VT) || (c == XOF)) 
				mconflush();	/* if ^C, ^S, ^K, flush first */

			if (head != tail) {	/* if not full, */
				conbuf[head]= c;/* char into buffer, */
				head= ++head % sizeof(conbuf); /* wrap pointer, */
			}
		}
	}
}
/* Check the time limit, and if over, bump the guy off. */

limitchk() {

	if (limit > 0) {
		if ((limit - minutes < 3) && ! wlimit) {

/* Set wlimit FIRST! to prevent this from recursing. */
			wlimit= 1;

			mprintf("\r\n\07You have only %u minutes left\07\r\n",
				limit - minutes);
		}
		if (minutes > limit) {

/* Set limit= 0 FIRST! to prevent recursion. */
			limit= 0;

			if (wover == 0) mprintf("\r\n\n\n\07Your time limit is up\r\n");
			goodbye("n");
		}
	}
}
/* Flush the console buffer. */

mconflush() {

	while (mconstat()) mconin();		/* flush all characters, */
	head= 1; tail= 0;			/* mark empty buffer */
}

/* Return true if console output ready. */

mconostat() {
	if (test) return(1);

	if (! dsr()) logoff(0);
	return(_mconostat());
}
/* Formatted print to the modem. */

mprintf(f)
char *f;
{
char buf[500];

	_spr(buf,&f);
	mputs(buf);
}

/* Handy function. */

mcrlf() {

	mputs("\r\n");
}
/* Formatted print to the log file. */

lprintf(f)
char *f;
{
char buf[500];
	_spr(buf,&f);
	_xwrite(cmtfile,buf,strlen(buf));
}

/* Save the character in the logfile. */

log(c)
char c;
{
	if (logflag)
		_xwrite(logfile,&c,1);
	return;
}
/* ----------------------------------------------------------------------- */

/* Write a string to the modem. */

mputs(s)
char *s;
{
	while (*s) mconout(*s++);
}
/* Return true if a modem character is available. */

modstat() {

	if (test) return(0);		/* cant XTELINK thru console */

	if (! dsr()) logoff(0);
	return(_mconstat());
}

/* Output to the modem. */

modout(c)
char c;
{
	if (test) return;		/* cant XTELINK in test mode */

	while (! _mconostat()) {
		if (! dsr()) logoff(0);
	}
	_mconout(c);
	return;
}
/* Get a character from the modem, with a maximum wait time, in
centiseconds. Returns -1 if timeout. */

int modin(hundredths)
unsigned hundredths;
{
long dly;

	if (test) return(-1);		/* cant in test mode */
	
	dly= hundredths * 10L;		/* time, in milliseconds */
	millis2= 0L;			/* clear the ticker, */
	while (millis2 < dly) {
		if (! dsr()) logoff(0);	/* watch disconnect, */
		if (_mconstat()) 	/* if a char avail, return it. */
			return(_mconin());
	}
	return(-1);			/* timeout */
}

/* Delay N centiseconds. Do nothing in the mean time. */

delay(n)
int n;
{
long dly;

	dly= n * 10L;
	millisec= 0L;
	while (millisec < dly);
}
/* Return true if carrier detect is true or "ignore CD" is true. */

dsr() {

	return(cd_flag ? 1 : _dsr() );
}

/* After DSR goes true, do whatever necessary to make the modem useful:
set baud rates, etc. NOTE: This calls mconin() and mconstat() instead
of the local ones here. These two are defined elsewhere, and watch for
DSR falling, etc in case of a hang up. */

connect() {

unsigned i;
char c;

	rate= 0;
	baud(rate);				/* set 300 baud, */
	while (modin(10) != -1)			/* wait for quiet line */
		limitchk();			/* (modem results, etc) */

	while (1) {
		while ((i= modin(1)) == -1)	/* wait for a key */
			limitchk();
		if ((i & 0xfc) == 0xfc) {	/* if all upper bits set, */
			rate= 1;		/* assume 1200 baud */
			break;
		} else if ((i & 0x7f) == CR) {/* if CR, then 300, */
			rate= 0;
			break;
		}
	}
	baud(rate);				/* set the found rate, */
	mconflush();				/* flush garbage, */
}
/* Get an input string; return NULL if error or empty line. Provide the
usual minimum editing capabilities. Echo as underscores if echo == 0. Always
clears the abort flag before exiting. */

#define LINELEN  74	/* max line length, */

getstring(string)
char string[];
{
int count;
char c;
char *p;
int pi;

	count= 0;
	line= 0;				/* clear current line counter, */
	while (1) {
		c= mconin();			/* get a key, */
		switch (c) {
		case CR:			/* process it, */
			string[count]= '\0';	/* terminate string, */
			abort= 0;
			return(count);		/* return string length */
			break;

		case BS:
		case DEL:			/* delete character */
		case XOF:
			if (count) {
				--count;	/* one less char, */
				mconout(BS);
				mconout(' ');
				mconout(BS);
			}
			break;
		case CAN:
		case ETX:
		case NAK:			/* delete line */
		case EM:
			while (count) {
				--count;
				mconout('\010');
				mconout(' ');
				mconout('\010');
			}
			break;
		case EOT:			/* retype character, */
			if (string[count])
				mcecho(string[count++]);
			break;
		case DC2:			/* retype line, */
			while (string[count]) {
				mcecho(string[count++]);
			}
			break;

		default:			/* insert character */
			if ( (c > 0x1f) && (c < DEL) && (count < LINELEN) ||
				(c == TAB)) {
				string[count++]= c;
				mcecho(c);
				if (count == (LINELEN - 8)) mconout(BEL);

			} else if (count >= LINELEN)
				mprintf("\n\\\b\07");
			else mconout(BEL);
			break;
		}
	}
}
/* Type the character, else an underscore if echo is off. */

mcecho(c)
char c;
{
	if (echo) mconout(c);
	else {
		mconout('.');
		log(c);
	}
}
/* Send any modem initialization file to the modem. */

init_modem(name)
char *name;
{

int f;
char ln[80],*p;

	f= _xopen(name,0);			/* if no file exists, */
	if (f == -1) return;			/* just exit */

	cd_flag= 1;				/* ignore carrier detect */
	while (_xline(f,ln,sizeof(ln) - 1)) {	/* pump out each line */
		p= ln;
		while (*p) modout(*p++);
		modout(CR);			/* send a CR, */
		while (modin(300) != -1);	/* gobble up echo, etc */
	}
	cd_flag= 0;				/* watch CD now */
	_xclose(f);
}
