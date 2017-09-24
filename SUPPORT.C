#include <stdio.h>
#include <ctype.h>
#include <bbs.h>

/* Misc. low level support routines. */


/* Display a message with the extention .BBS. */

bbsmsg(filename)
char *filename;
{
char name[82];

	strcpy(name,sys-> bbspath);		/* assemble the file, */
	strcat(name,filename);
	strcat(name,".bbs");
	return(message(name));			/* display it, */
}
/* Display a message with the extention .BBS from the Message path */

msgmsg(filename)
char *filename;
{
char name[82];

	strcpy(name,sys-> msgpath);		/* assemble the file, */
	strcat(name,filename);
	strcat(name,".bbs");
	return(message(name));			/* display it, */
}
/* Display a message with the extention .HLP. */

hlpmsg(filename)
char *filename;
{
char name[82];

	strcpy(name,sys-> hlppath);		/* assemble the file, */
	strcat(name,filename);
	strcat(name,".hlp");
	return(message(name));			/* display it, */
}
/* General purpose message file displayer. Return true if no file. */

message(filename)
char *filename;
{
int file;
int s,i;

	file= _xopen(filename,0);
	if (file == -1)
		return(1);

	while (s= _xread(file,_txbuf,_TXSIZE)) {
		for (i= 0; i < s; i++) {	/* print it, */
			if (abort) break;	/* watch ^C, */
			if (_txbuf[i] == 0x1a) break;
			mconoutr(_txbuf[i]);	/* note mconoutR() */
		}
		if (abort) break;
	}
	if (abort) mcrlf();

	_xclose(file);
	return(0);
}
/* System file access. Read (0) or write (1) the specified system
file. This is the only .BBS files that does not use a path, but must reside
in the default directory. If the file does not exist create it. Systm file
#0 is special; it is called merely "system.bbs", for backwards compatibility,
and also where the total number of callers is kept. */

system(f,n)
int f,n;
{
int sysfile;
char name[40];

	strcpy(name,"system.bbs");		/* system file 0, */
	if (n > 0) sprintf(name,"system%u.bbs",n); /* build the name, */
	sysfile= _xopen(name,2);
	if (sysfile == -1) {			/* if it doesnt exist, */
		sysfile= _xcreate(name,2);	/* create it, */
		sys-> caller= 0;		/* clear things, */
		sys-> priv= NORMAL;		/* set privelege */
		sys-> attrib= 0;
		f= 1;				/* force it to be written, */
	}
	if (f) {
		_xwrite(sysfile,sys,sizeof(struct _sys)); /* update it, */
	} else {
		sys-> attrib= 0;		/* for older versions */
		_xread(sysfile,sys,sizeof(struct _sys)); /* read the good stuff, */
	}
	_xclose(sysfile);			/* close it up, */
	sysnum= n;				/* save number in global, */
}
/* Count the number of active FILE and MSG paths. */

syscount() {

int i;
char name[14];
long fsize;

	i= 0;
	while (_xfind("system*.bbs",name,&fsize,0,i)) ++i;
	sysfiles= i - 1;
}

stolower(s)
char *s;
{
	while (*s) {
		*s= tolower(*s);
		++s;
	}
}
stoupper(s)
char *s;
{
	while (*s) {
		*s= toupper(*s);
		++s;
	}
}
/* Print an unsigned int, followed by st, rd, th, etc., and a comma after
the 1000's digits. */

nth(n)
unsigned n;
{

	if (n > 999) {
		mprintf("%u,",n / 1000);
		n %= 1000;
		if (n < 100) mprintf("0"); /* do zero fill since we */
		if (n < 10) mprintf("0"); /* just chopped the digits */
	}
	mprintf("%u",n);
	switch(n % 100) {		/* check for all the 'teens, */
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
		case 17:
		case 18:
		case 19:
			mprintf("th");
			return;
			break;
	}

	switch(n % 10) {
		case 1:
			mprintf("st");
			break;
		case 2:
			mprintf("nd");
			break;
		case 3:
			mprintf("rd");
			break;
		default:
			mprintf("th");
			break;
	}
}
/* Display an integer as dollars and cents. The value passed is assumed to
be integer cents. */

dollar(n)
int n;
{
	mprintf("$%u.",n / 100);		/* whole dollars, */
	astwo(n % 100);				/* two digits cents */
}

/* Set the privelege level. */

setpriv(s,v)
char *s;
int *v;
{
	stolower(s);
	if (strcmp(s,"twit") == 0) *v= TWIT;
	if (strcmp(s,"disgrace") == 0) *v= DISGRACE;
	if (strcmp(s,"normal") == 0) *v= NORMAL;
	if (strcmp(s,"privel") == 0) *v= PRIVEL;
	if (strcmp(s,"extra") == 0) *v= EXTRA;
	if (strcmp(s,"sysop") == 0) *v= SYSOP;
}
/* Display the current privelege level. */

getpriv(n)
int n;
{
	switch(n) {
		case TWIT:
			mprintf("Twit");
			break;
		case DISGRACE:
			mprintf("Disgrace");
			break;
		case NORMAL:
			mprintf("Normal");
			break;
		case PRIVEL:
			mprintf("Privel");
			break;
		case EXTRA:
			mprintf("Extra");
			break;
		case SYSOP:
			mprintf("Sysop");
			break;
		default:
			mprintf("BAD PRIV = %d",n);
			break;
	}
}
/* Display time as hh:mm, from the absolute minutes-since-midnite. */

min_time(n)
int n;
{
	astwo(n / 60);
	mconout(':');
	astwo(n % 60);
}
/* Display a number as two digits, with leading zeros as neccessary. */

astwo(n)
int n;
{
	if (n < 10) mconout('0');
	mprintf("%u",n);
}
/* Display the time. The value passed is hh:mm or mm:ss, in either
hours * 60 * minutes or minutes * 60 + seconds. */

cmin_time(n)
int n;
{
	astwo(n / 60);
	lconout(':');
	astwo(n % 60);
}
/* Display a number as two digits, with leading zeros as neccessary. */

castwo(n)
int n;
{
	if (n < 10) lconout('0');
	cprintf("%u",n);
}
/* Same as min_time, except to the log file. */

lmin_time(n)
int n;
{
	lastwo(n / 60);
	lprintf(":");
	lastwo(n % 60);
}
/* Display a number as two digits, with leading zeros as neccessary. */

lastwo(n)
int n;
{
	if (n < 10) lprintf("0");
	lprintf("%u",n);
}
