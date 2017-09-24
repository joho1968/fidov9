#include <stdio.h>
#include <ctype.h>
#include <bbs.h>

/* Signon and password section of BBS.

1/	Asks for your name. Must be one or two words, and less than 36
	characters long. The name is cleaned up: no leading spaces, all
	lower case, capitalized 1st letter, one seperating space, no
	trailing spaces. Verifies that the name is correct.

2/	Looks in the user file for the name. (Error if file not found.)
	If found, increments the number of times called. Seeks the file
	back to the start of that record. If not found, blabs about using
	the help command, creates a new record.

3/	If 'bbs' is false, aborts if the name doesnt exist. 

*/

signon() {

char name[36],lname[36],pwd[20];
int i,eof,found,f;
unsigned llimit;
char c;

	bbsmsg("welcome1");
	*name= '\0';

/* Get the guys name. If he enters both halfs, then dont ask for the
last name nor confirmation. */

	do {
		getfield(name,"Your FIRST name: ",1,2,18,1);
		if (num_args(name) < 2) {
			getfield(lname,"Your  LAST name: ",0,1,18,1);
			if (strlen(lname) > 0) {
				strcat(name," "); 
				strcat(name,lname);
			}
		} 
	} while (!verify(name));

/* Look for the guy in the user list. Update the list if we find him,
else create a new entry. */

	eof= 0; found= 0;			/* search the list, */
	mprintf("Wait ...\r\n");

	f= _xopen("user.bbs",0);
	if (f == -1) {
		mprintf("ERROR: Cant find the user list\r\n");
		doscode= 1;
		frc_abort();
	}
	userpos= 0L;				/* user recd position in file */
	while (!eof && !found) {
		if (_xread(f,user,sizeof(struct _usr)) != sizeof(struct _usr)) {
			eof= 1;
		} else if (strcmp(name,user-> name) == 0) {
			found= 1;
		} else 
			userpos+= sizeof(struct _usr);
	}
	_xclose(f);

	if (found) {
		llimit= nlimit;			/* normal user, */

	} else {				/* new caller */
		llimit= first_limit;		/* time limit, */
		user-> help= NOVICE;		/* set defaults, */
		user-> tabs= 0;			/* dont expand tabs, */
		user-> nulls= 0;		/* 0 nulls per linefeed, */
		user-> width= 80;		/* screen width, */
		user-> len= 24;			/* screen length */
		gtod(user-> date);		/* todays date, */
		user-> times= 0;		/* 1st time calling, */
		user-> msg= 1;			/* initially msg area 1, */
		user-> files= 1;		/* initially file area 1, */
		user-> flag= 0;
		user-> more= 0;			/* dont pause after 24 lines, */
		user-> priv= def_priv;		/* default privelege, */
		user-> dnld= 0;			/* no file xfer yet, */
		user-> upld= 0;
		user-> dnldl= 0;
		user-> time= 0;			/* no time on yet */
		strcpy(user-> name,"");		/* clear name, etc */
		strcpy(user-> city,"");
		strcpy(user-> pwd,"");
		if (bbs) {			/* if public system, */
			bbsmsg("newuser1");	/* ask for stuff, */
			do {	mconflush();
				getfield(user-> city,"Where are you calling from? (City, ST) ",0,3,35,1);
			} while (!verify(user-> city));

			do {	mconflush();
				getfield(user-> pwd,"Pick a password: ",1,1,14,1);
			} while (!verify(user-> pwd));

			user-> more= 1;		/* force pause every 24 lines, */
			line= 0;
			abort= 0;
			bbsmsg("newuser2");

			strcpy(user-> ldate,""); /* no last time called, */
			user-> credit= 0;
			user-> debit= 0;
			for (i= 0; i < sizeof(user-> name); i++)
				user-> name[i]= '\0';
			strcpy(user-> name,name);
			found= 1;			/* mark as "found user" */
		}
	}
	if (found) {
		found= 0;			/* REUSE OF 'FOUND' */
		for (i= 0; (i < 3) && !found; i++) {
			echo= 0;		/* stop echo, */
			getfield(pwd,"Password: ",1,1,15,1);
			echo= 1;		/* enable again, */
			if (strcmp(pwd,user-> pwd) == 0) found= 1;
			else mprintf("Wrong!\r\n");
		}
		if (found) {			/* if password OK, */
			if (*(user-> ldate))	/* if called in before, */
				mprintf("You last called on %s\r\n",user-> ldate);
			limit= llimit;		/* set real time limit */
			return(1);		/* run BBS, */
		}
	}

/* Either not a BBS (private system) or idiot forgot his password. If there
is a questionaire, fill it in, else let the guy leave a message to the sysop. */

	if (question("qnopwd.bbs","anopwd.bbs") == 0) {
		bbsmsg("nopwd");		/* "no password" spiel, */
		cmtflag= 1;			/* allow comments to be written, */
		goodbye("y");			/* logoff, ask for a message. */
	}
	goodbye("n");				/* logoff questionaire */
}
/* Given a prompt string, get a yes or no answer. Yes == 1. */

verify(s)
char *s;
{
char c;

	do {	mprintf("%s, correct? (y,n) ",s);
		c= mconecho();
		mcrlf();
		c= tolower(c);
	} while ((c != 'y') && (c != 'n'));
	return(c == 'y');
}
/* Fielded input. Inputs are:

string		where to put the input,
prompt		the prompt string,
min		minimum number of words,
max		maximum number of words,
width		maximum string size,
cap		Remove extra spaces, capitalize, etc. Works
		only if one or two words.
*/

getfield(string,prompt,gmin,gmax,width,cap)
char *string,*prompt;
int gmin,gmax,width,cap;
{
char buff[82];
int i;
char *p;
	while (1) {
		mprintf("%s",prompt);
		getstring(buff);
		mcrlf();
		if (strlen(buff) > width) {
			mprintf("Too wide!\r\n");
			for (i= 0; i < (strlen(prompt) - 1); i++)
				mconout(' ');
			mconout('|');
			for (i= 0; i < width; i++)
				mconout('-');
			mprintf("|\r\n");
			continue;
		}

		if ((num_args(buff) > gmax) || (num_args(buff) < gmin)) {
			if (gmin != gmax) mprintf("Must be %d to %d words.\r\n",gmin,gmax);
			else mprintf("Must be %u word(s)\r\n",gmin);
			continue;
		}
		if (cap) {
			p= skip_delim(buff);
			stolower(p);			/* make all lower case, */
			cpyarg(string,p);		/* copy in 1st name, */
			string[0]= toupper(string[0]);	/* capitalize, */
			p= next_arg(p);			/* skip that one, */
			while (num_args(p)) {
				strcat(string," ");	/* separate with a space, */
				*p= toupper(*p);
				cpyarg(&(string[strlen(string)]),p); /* add 2nd name, */
				p= next_arg(p);		/* ptr to next name, */
			}
		} else
			strcpy(string,buff);
		break;
	}
}
/* Local handy subroutine: input a character, echo it. */

mconecho() {

char c;
	c= mconin();
	mconout(c);
	return(c);
}
