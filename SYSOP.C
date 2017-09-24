#include <stdio.h>
#include <ctype.h>
#include <bbs.h>

/*
	Rolodex cum sysop utility

*/

long _xseek();

/* Global stuff */

unsigned recd;			/* current record, */
int usr;			/* index file handle, */
int hlp;			/* help file handle */
int changed;			/* true if record has been changed */
char linebuf[81];		/* local line editing, */
char fname[80];			/* rolodex file name, */
unsigned _stack = 32765;	/* for autos and stack, */
unsigned file_space = 1000;	/* space for file I/O, */

unsigned txptr;
unsigned txcount;

main(argc,argv)
int argc;
char *argv[];
{
int i;
char buff[20],*p,c;
int doinit;
char path[80];
long m,a;

	allmem();
	m= sizmem();				/* total mem size, */
	a= (long)_stack + (long)file_space;	/* total up requirements */
	a+= sizeof(struct _usr) * 2;
	a+= _TXSIZE;

	p= getml(m - (long)_stack - (long)file_space);

	user= (struct _usr *)p;		/* make all our structures, */
	p+= sizeof(struct _usr);
	_txbuf= p;

	doinit= 1;
	fname[0]= '\0';

	while (--argc) {
		strip_switch(buff,*++argv);	/* get the switches, */
		p= buff;
		while (*p) {			/* /R means no init */
			if (*p == 'R') doinit= 0; /* the modem */
			++p;
		}
		cpyarg(fname,*argv);		/* possible filename */
	}
	if (*fname == '\0') {			/* if blank, */
		strcpy(fname,"user.bbs");	/* default file, */
	}
	strcpy(path,"sysop.hlp");	/* help file name, */
	hlp= _xopen(path,0);		/* dont check errors here */

	scrinit();			/* set screen stuff etc */
	place(bottom,0);
	for (i= 25; i--;) lconout('\n'); /* clear screen, */
	usr= _xopen(fname,2);		/* open it, */
	if (usr == -1) {		/* usual error handling, */
		cprintf("\rCant find %s\r\n",fname);
		exit();
	}
	gtod(date);			/* todays date */
	menu();				/* paint the menu, */
	stat("Fido's Sysop Utility 10 July 84");
	recd= 1;			/* open first record, */
	get(recd);
	changed= 0;			/* obviously not changed yet */
	while (1) {
		display();		/* display record, */
		place(0,0);
		cputs("Command: "); 	/* prompt, etc */
		clreol();
		do c= lconin();
		while ((c == 13) || (c == ' '));
		process(c);		/* execute commands, */
	}
}
/* Process commands. */

process(c)
char c;
{
char work[80],d,*p;
int recd_save,found,i;

	switch(tolower(c)) {
		case 'q':
			if (changed) upd();
			_xclose(usr);	/* close the file, */
			_xclose(hlp);	/* close help file */
			place(bottom,0); clreol();
			place(bottom - 1,0); clreol();
			exit();
			break;
		case 'n':
			getfield(user-> name,"Name: ",0,2,36,1);
			changed= 1;
			break;
		case 'c':
			getfield(user-> city,"City, ST: ",0,3,36,1);
			changed= 1;
			break;
		case 'p':
			getfield(user-> pwd,"Password: ",0,1,16,1);
			changed= 1;
			break;
		case '?':
			place(0,0); cprintf("Help with what command (? for general help): ");
			d= lconin(); 
			if ((d == 13) || (d == 27)) break;
			lconout(d);
			stat("HELP!");
			help(d);
			menu();
			break;

		case '=':		/* same key */
		case '+':
		case '>':
		case '.':
			if (changed) upd();
			stat("Next");
			if (get(recd + 1)) ++recd;
			else stat("Last One");
			break;
		case '_':		/* same key */
		case '-':
		case '<':
		case ',':
			if (changed) upd();
			stat("Previous");
			if (recd > 1) {
				if (get(recd - 1)) --recd;
				else stat("First One");
			} else stat("First One");
			break;
		case 'e':
			if (changed) upd();
			stat("Enter New");
			while (get(recd)) ++recd;
			clear();
			break;
		case 'z':
			if (changed) upd();
			stat("End");
			while (get(recd + 1)) ++recd;
			get(recd);
			break;
		case 'b':
			if (changed) upd();
			stat("Beginning");
			recd= 1;
			get(recd);
			break;
		case 'f':
			if (changed) upd();
			stat("Find Someone");
			enter("Who to look for",linebuf,20);
			if (strlen(linebuf) == 0) break;
			find(linebuf);
			break;
		case 'd':
			user-> flag |= 1;
			changed= 1;
			stat("Deleted");
			break;
		case '!':
			if (changed) upd();
			stat("Purge Deleted");
			purge();
			break;

		case 'o':
			stat("Mark Old Users as Deleted");
			place(0,0); clreol();
			getfield(work,"Delete Users how many days old: ",0,1,10,1);
			i= atoi(work);
			if (i < 2) {
				stat("Aborted");
				break;
			}
			mark_old(i);
			break;
		case 'i':
			i= recd;			/* save current, */
			while (1) {
				if (get(++recd) == 0) {	/* wrap to BOF, */
					recd= 0;	/* (preincrement) */
					continue;
				}			/* if we hit the starting */
				if (user-> flag & 1) break;
				if (recd == i) break;	/* point stop */
				if (keyhit() == 27) break;
				sprintf(work,"Recd #%u",recd);
				stat(work);
			}
			break;			
		case 'a':
			if (changed) upd();
			stat("Position to Record Number");
			getfield(work,"Enter Record Number 1 - N: ",0,1,10,0);
			if (atoi(work) == 0) break;
			if (get(atoi(work))) recd= atoi(work);
			else stat("No such Record");
			break;

		case 'm':
			stat("Change Default Msg Area");
			getfield(work,"Enter Area Number 1 - N: ",0,1,10,0);
			if (atoi(work) == 0) break;
			changed= 1;
			user-> msg= atoi(work);
			break;

		case 'k':
			stat("Change Default File Area");
			getfield(work,"Enter Area Number 1 - N: ",0,1,10,0);
			if (atoi(work) == 0) break;
			changed= 1;
			user-> files= atoi(work);
			break;

		case 'v':
			changed= 1;
			stat("Change Privelege");
			getfield(work,"Privelege (Twit, Disgrace, Normal, Priveleged, Extra, Sysop): ",0,1,1);
			switch (*work) {
				case 'T':
					user-> priv= TWIT;
					break;
				case 'D':
					user-> priv= DISGRACE;
					break;
				case 'N':
					user-> priv= NORMAL;
					break;
				case 'E':
					user-> priv= EXTRA;
					break;
				case 'P':
					user-> priv= PRIVEL;
					break;
				case 'S':
					user-> priv= SYSOP;
					break;
			}
			changed= 1;
			break;

		case '$':
			stat("Set Credit (clear debit)");
			getfield(work,"Enter add'tl credit (dollars): ",0,1,10,0);
			p= work;
			if (*p == '=') {
				user-> credit= 0;
				++p;
			}
			i= atoi(p) * 100;
			user-> credit+= atoi(p) * 100;
			user-> debit= 0;
			changed= 1;
			break;

		default:
			while (keyhit());	/* error, flush keybd */
			break;
	}
}
/* Search for the specified name. */

find(pattern)
char *pattern;
{
int recd_save,found,i;
char work[80];

	recd_save= recd;			/* save current, */
	found= 0;				/* not found yet */
	while (!found) {			/* stop if at start */
		if (get(++recd) == 0) {		/* wrap to BOF, */
			recd= 0;		/* (preincrement) */
			continue;
		}				/* if we hit the starting */
		if (recd == recd_save) break;	/* point stop */
		if (keyhit() == 27) break;
		sprintf(work,"Searching Recd #%u",recd);
		stat(work);
		if (substr(pattern,user-> name)) {
			found= 1;
			break;
		}
	}
	if (found) stat("Found It");
	else stat("Cant Find It");
}
/* Mark all records that are over n days old. */

mark_old(n)
int n;
{
int recd_save,marked,i;
char work[80];

	marked= 0;
	recd_save= recd;			/* save current, */
	while (1) {
		if (get(++recd) == 0) {		/* wrap to BOF, */
			recd= 0;		/* (preincrement) */
			continue;
		}				/* if we hit the starting */
		if (recd == recd_save) break;	/* point stop */
		if (keyhit() == 27) break;
		sprintf(work,"Checking Recd #%u",recd);
		stat(work);

		if ((days(user-> ldate,date) > n) && (user-> credit == 0) && (user-> priv < SYSOP)) {
			display();
			user-> flag |= 1;	/* mark deleted */
			++marked;
		} else user-> flag &= 0x00;	/* else unmark */

		put(recd);
	}
	sprintf(work,"Marked %u as deleted",marked);
	stat(work);
}
/* Purge all deleted records. Works by copying to a temp file, except the
deleted ones, then renaming. */

purge() {

int t,o,n;
char s[80];

	t= _xcreate("user$$$$.$$$",2);
	if (t == -1) {
		stat("Cant create temp file!");
		return;
	}
	o= _xcreate("user.old",2);
	if (o == -1) {
		stat("cant create old user file");
		return;
	}
	n= 0;
	recd= 0;
	while (get(++recd)) {
		if (((user-> flag & 1) == 0) || (user-> priv >= EXTRA)) {
			user-> flag &= 0x00;
			if (_xwrite(t,user,sizeof(struct _usr)) != sizeof(struct _usr)) {
				stat("Disk Full");
				_xclose(t);
				_xclose(o);
				return;
			}
		} else {
			user-> flag= 0x00;	/* unmark deleted */
			if (_xwrite(o,user,sizeof(struct _usr)) != sizeof(struct _usr)) {
				stat("Disk Full");
				_xclose(t);
				_xclose(o);
				return;
			}
			++n;
		}
	}
	_xclose(t);
	_xclose(o);
	_xclose(usr);
	_xdelete("user.bak");
	_xrename("user.bbs","user.bak");
	_xrename("user$$$$.$$$","user.bbs");
	sprintf(s,"Purged %u recds",n);
	stat(s);
	usr= _xopen("user.bbs",2);
	if (usr == -1) {
		stat("Cant reopen user.bbs");
		exit();
	}
	recd= 1;
	get(recd);
	changed= 0;
}

/* Local function: if the record has changed, write it out. */

upd() {
	if (changed) {
		stat("Saving First");
		setdate();
		put();
		changed= 0;
	}
}
/* Display the menu. */

menu() {
int i,j,n;

	i= 1;
	place(++i,2); cputs("+ ... Next          - ... Previous    B ... Beginning     Z ... End"); 
	place(++i,2); cputs("N ... Name          C ... City        P ... Password      V ... Privelege"); 
	place(++i,2); cputs("F ... Find a Name   D ... Delete      O ... Delete Old    ! ... Purge Deleted"); 
	place(++i,2); cputs("M ... Msg Area      K ... File Area   A ... Pick Recd #   $ ... Set Credit"); 
	place(++i,2); cputs("E ... Enter New     Q ... Quit        ? ... Help"); 
	place(++i,47); cputs("Fido's SYSOP Utility 10 July 84");
	box(1,++i);
	place(i,30); cputs(" COMMANDS ");

	j= ++i;
	place(i,0);

	for (n= i; n < bottom; n++) {
		clreol();
		lconout('\n');
	}
	place(++i,8); cputs("RECORD #");
	place(++i,8); cputs("FIRST CALLED:                        LAST CALLED:"); 
	place(++i,8); cputs("TIMES CALLED:");
	place(++i,8); cputs("NAME:");
	place(++i,8); cputs("PASSWORD:");
	place(++i,8); cputs("CITY:");
	place(++i,8); cputs("MSG AREA:         FILE AREA:");
	place(++i,8); cputs("UPLOADED:         DOWNLOADED:");
	place(++i,8); cputs("CREDIT:           DEBIT");
	place(++i,8); cputs("PRIVELEGE:");
	box(j,++i);
	place(i,30); cputs(" USER RECORD ");
}
/* Display a status string. */

stat(s)
char *s;
{
int i;
	place(1,10); lconout(' '); cputs(s); lconout(' ');
	for (i= 30; --i;) lconout(horizbar);
}
/* Display the current record on the screen. */

display() {

int i,j,t;

	i= 9;
	place(++i,16); cprintf("%-4u",recd);  
	place(++i,21); cprintf("%20s",user-> date);
	place(i,58); cprintf("%20s",user-> ldate);
	place(++i,22); cprintf("%-4u",user-> times); 
	place(++i,14); cprintf("%-20s",user-> name);  
	place(++i,18); cprintf("%-20s",user-> pwd); 
	place(++i,14); cprintf("%-36s",user-> city);  
	place(++i,18); cprintf("%-2u",user-> msg);
	place(i,37); cprintf("%-2u",user-> files);
	place(++i,18); cprintf("%6uK",user-> upld);
	place(i,38); cprintf("%6uK",user-> dnld);
	place(++i,16); dollar(user-> credit);
	place(i,32); dollar(user-> debit);
		      
	place(++i,19);
	 switch(user-> priv) {
		case TWIT:
			cputs("Twit    ");
			break;
		case DISGRACE:
			cputs("Disgrace");
			break;
		case NORMAL:
			cputs("Normal  ");
			break;
		case PRIVEL:
			cputs("Privel  ");
			break;
		case EXTRA:
			cputs("Extra   ");
			break;
		case SYSOP:
			cputs("Sysop   ");
			break;
		default:
			cputs("????????");
			break;
	 }
}

/* Clear the note record. */

clear() {
int i;

	*user-> ldate= '\0';
	user-> city[0]= '\0';
	user-> name[0]= '\0';
	user-> pwd[0]= '\0';
	user-> times= 0;
	user-> help= 1;
	user-> tabs= 0;
	user-> nulls= 0;
	user-> msg= 1;
	user-> more= 1;
	user-> ldate[0]= '\0';
	user-> flag= 0;
	user-> upld= 0;
	user-> dnld= 0;
	user-> dnldl= 0;
	user-> files= 1;
	user-> priv= NORMAL;
	user-> time= 0;
	user-> credit= 0;
	user-> debit= 0;
	setdate();
}
/* Set the current date into the record. */

setdate() {

	gtod(user-> date);
}
/* Enter text into a field, given a prompt, the array, and 
the maximum length. */

enter(prompt,s,len)
char *prompt,*s;
int len;
{
int i;

	place(0,0); cprintf("%s: ",prompt); clreol();
	strcpy(linebuf,s);
	getstring(linebuf);
	if (strlen(linebuf) == 0) return;
	for (i= 0; i < len - 1; i ++) {
		s[i]= linebuf[i];
		if (linebuf[i] == '\0') break;
	}
	s[i]= '\0';
}
/* Position to record. NOTE: Does not check (nor can it) for seeking
beyond the end. EOF gets checked by get() when it reads. */

posn(n)
unsigned n;
{
long pos,actual;

	pos= (long) (n - 1) * (long) sizeof(struct _usr);
	actual= _xseek(usr,pos,0);
	return(pos == actual);
}
/* Read in the specified record, leave the file pointer pointing to it.
Return 0 if not found. */

get(n)
int n;
{
int cnt;
	posn(n);				/* position, */
	cnt= _xread(usr,user,sizeof(struct _usr));/* read it, */
	_xseek(usr, (long) - cnt, 1);		/* seek back, */
	return (cnt == sizeof(struct _usr));
}
/* write the record out to the current position. Leaves the pointer at 
the current record. */

put() {
int cnt;
	cnt= _xwrite(usr,user,sizeof(struct _usr));
	_xseek(usr, (long) - cnt,1);
	return(cnt == sizeof(struct _usr));
}

/* Help. Display the help screen for the specified command. */

help(c)
char c;
{
char line[132];
int cnt,i,t;

	if (hlp == -1) {
		stat("No help file");
		return;
	}
	c= tolower(c);				/* easier testing */
	_xseek(hlp,0L,0);			/* start of file, */
	txcount= 0;				/* force disk read, */
	txptr= 0;

	place(0,0); cprintf("Wait ..."); clreol();

	while (cnt= getline(hlp,line,sizeof(line))) {
		if ((line[0] == '\\') && (tolower(line[1]) == c)) break;
	}
	if (cnt == 0) {
		stat("Cant Help with that");

	} else {
		t= 9;					/* top line, */
		for (i= t + 1; i < bottom; i++) {
			if (getline(hlp,line,sizeof(line) - 1) == 0) break;
			if ((*line == '\0') || (*line == '\\')) break;
			place(i,0); cputs("        "); cputs(line); clreol();
		}
		box(t,i);
		while (++i <= bottom) {
			place(i,0); clreol();
		}
	}
	place(0,0); cputs("Hit any key to continue: "); clreol();
	while (keyhit());
	lconin();
}
/* get a line of text into 'line', and null terminate it. Return the number
of characters read; 0 == end of file. */

getline(file,line,size)
int file;
char *line;
int size;
{
int i;
char c;
	i= 0;
	while (i < size - 1) {
		if ((c= getbc(file)) == 0) break;
		if ((c == '\r') || (c == 0x1a)) continue;
		line[i++]= c;
		if (c == '\n') break;
	}
	line[i]= '\0';				/* null terminate, */
	return(i);
}
/* Get a character from the buffer. Return 0 when no more. */

getbc(file)
int file;
{
	if ((txptr >= txcount) || (txcount == 0)) {
		txcount= _xread(file,_txbuf,_TXSIZE);
		if (txcount == 0) return(0);
		txptr= 0;
	}
	return(_txbuf[txptr++] & 0x7f);
}
