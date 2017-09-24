#include <stdio.h>
#include <ctype.h>
#include <mail.h>
#include <bbs.h>

/*
Message base commands for BBS

mread(p)	Read messages. User can specify the starting message, or
		New messages. The last message read is kept in the user's
		record. 

mkill(p)	Kill messages. Only allowed if either to or from the user-> 

mlist(p)	List messages.

mfind(p)	Search for a pattern; matches any field.

msgsel(n)	Select message base number N (0 - N) or prompt for it
		listing the possible selections.
*/
/* Select one of the message paths available. */

/* Change the default directory. If no arg entered, display DIR.BBS in each
active directory. Get the directory number. */

msgsel(n)
int n;
{
int i;
int l,m;
char path[80];

	if (user-> priv >= SYSOP) m= 0;			/* only sysops can get */
	else m= 1;					/* area 0 */

	l= msgnum;					/* save in case no change */
	if ((n < m) || (n > sysfiles)) {
		mprintf("----- Message Areas -----\r\n");
		abort= 0;

		for (i= m; i <= sysfiles; i++) {	/* list them all, */
			msg_dir(i);
			if (abort) break;
		}
	} else system(0,n);

	while ((n < m) || (n > sysfiles) || (strlen(sys-> msgpath) == 0) || (sys-> priv > user-> priv)) {
		mprintf("Message Area, or Quit: ");
		getstring(path);			/* get new dir, */
		mcrlf();
		if (cmd_abort(path))
			n= l;				/* if blank no change, */
		else n= atoi(path);			/* get number, */
		if (n <= sysfiles) system(0,n);		/* read that one, */
	} 
	msgnum= n;					/* select permanently, */
	system(0,msgnum);				/* read it, */
	if (msgnum != l) {				/* if it changed, */
		mprintf("Wait ...\r\n");
		msgcount();				/* count the messages, */
	}
	return(msgnum != l);
}
/* Display the dir.bbs file from message path N. Dont display it if null. */

msg_dir(n)
int n;
{
	system(0,n);			/* read the system file, */
	if ((sys-> priv > user-> priv) || (strlen(sys-> msgpath) == 0)) 
		return;

	stoupper(sys-> msgpath);	/* display pathname */
	mprintf("%2u ... %c %-14s",
	   n,(sys-> attrib & SYSMAIL ? '*' : ' '),sys-> msgpath);
	if (msgmsg("dir"))		/* display the directory description, */
		mcrlf();
}
/* Search for a string in any field of a message header. List the headers
of all matching messages. Searches from the highest to the lowest message. */

mfind(p)
char *p;
{
char pattern[82],thing[82],*match;
unsigned msgnbr,i,flag,messg[20];
int direction;

	if (strlen(p) == 0) {
		if (user-> help > EXPERT) mprintf("Search for name or subject.\r\n");
		getfield(pattern,"What to look for: ",0,1,sizeof(pattern) - 2,1);
		if (cmd_abort(pattern)) return;
	} else strcpy(pattern,p);		/* I break for idiots */
	stolower(pattern);			/* make pattern lower case, */

	i= 0;					/* none found, */
	direction= -1;				/* search backwards, */
	msgnbr= msg_highest;			/* search all messages, */

	mprintf("Looking at msg #");
	while (msgnbr= findmsg(msgnbr,direction)) {
		mprintf("%3u\010\010\010",msgnbr);
		_xclose(msgfile);		/* dont need it, */
		if (abort) break;

		flag= 0;			/* no match yet, */
		strcpy(thing,msg-> to);		/* local copy of object, */
		stolower(thing);		/* lower case for check, */
		if (stcpm(thing,pattern,&match)) flag= 1; /* check it, */

		strcpy(thing,msg-> from);	/* do for all portions */
		stolower(thing);		/* of the message header, */
		if (stcpm(thing,pattern,&match)) flag= 1;

		strcpy(thing,msg-> subj);	/* local copy of object, */
		stolower(thing);		/* lower case for check, */
		if (stcpm(thing,pattern,&match)) flag= 1; /* check it, */

		if (flag) {			/* if a match, save the */
			if (i < 20) messg[i++]= msgnbr;	/* msg number, */
			mcrlf();	/* fix "looking " */
			listhdr(msgnbr);	/* list it */
			mprintf("Looking at msg #"); /* re do it, */
		}
		msgnbr+= direction;		/* next message ... */
	}
	mcrlf();

	if (i) {
		mprintf("Messages: ");
		while (i--)
			mprintf("%u,",messg[i]);
		mcrlf();
	} else
		mprintf("Not found.\r\n");
	return;
}
/* List messages. The command tail can contain the starting number, or N
for New messages. If blank, prompt for it. */

mlist(p)
char *p;
{
int msgnbr;
int flag;	/* not used */
char work[82];

	if (user-> help > EXPERT) mprintf("Summary of messages.\r\n");
	sprintf(work,"Starting # (1 to %u)",msg_highest);
	if ((msgnbr= numfield(p,work,&flag)) == 0)
		return;

	if (flag == 0) flag= 1;		/* default to read forward, */
	while (msgnbr= findmsg(msgnbr,flag)) {
		_xclose(msgfile);
		listhdr(msgnbr);
		mcrlf();
		if (abort) break;
		msgnbr+= flag;
	}
	return;
}
/* Read one or more messages. If multiple messages (*, n+, n-) then a pause
is done after every message, and the message can be killed, skipped, or
replied to. Reply to merely enters a message to the address of the read 
message. */

mread(p)
char *p;
{
int msgnbr;
int n;
int flag;

	if (user-> help > EXPERT) mprintf("Read messages.\r\n");

	flag= -1;
	msgnbr= msg_highest;
	while (1) {
		n= mquery(p,msgnbr,&flag);
		if (n < 0) continue;		/* again if error, help, etc */
		if (n == 0) break;		/* stop from mquery() */
		if (n= findmsg(n,flag)) {	/* try to read it, */
			msgnbr= n;		/* sucessful, */
			type_msg(msgnbr);	/* display it, */
		} else {
			if (msg-> attr & MSGPRIVATE) mprintf("Private Message\r\n");
			else mprintf("No such message\r\n");
		}
		if (abort) p= "";		/* flush args if abort, */
		p= next_arg(p);			/* maybe more than one */
	}
}
/* Type the contents of the open message file, then close it. Abort 
if ^C typed. Increment the number of times read field. */

type_msg(msgnbr)
int msgnbr;
{
int s,i;

	listhdr(msgnbr);
	listtext();
	++msg-> times;				/* read once more, */
	if (strcmp(user-> name,msg-> to) == 0) msg-> attr |= MSGREAD;
	wrtmsg();				/* update it, */
}
/* List the message text. Wrap words to the users screen width. */

listtext() {

int screen,n,v,i,width;
char word[80],c;

	width= user-> width;
	if (width > 72) width= 72;

	screen= 0;
	v= n= 0;

	while (!abort) {
	
		i= 0;
		word[i]= '\0';
		while (strlen(word) < sizeof(word) - 2) {
			if (n == v) {
				v= _xread(msgfile,_txbuf,_TXSIZE);
				n= 0;
				c= '\0';		/* in case EOF */
			}
			if (v == 0) break;

			c= _txbuf[n++];
			if (c == '\n') continue;	/* ignore LFs, */
			if (c == '\r') break;		/* stop if CR, */
			if (c == '\r' + 128) continue;	/* skip soft CRs */
			word[i++]= c;			/* store others, */
			word[i]= '\0';
			if ((c == ' ') || (c == ',')) break;
		}

		if (strlen(word) + screen > width) {
			mcrlf();		/* too long; wrap */
			screen= 0;			/* before typing word */
		}
		mprintf("%s",word);			/* display word */
		screen+= strlen(word);			/* size on screen */
		if (c == '\r') {			/* if a hard CR, */
			mcrlf();		/* do a CR LF */
			screen= 0;
		}
		if (c == '\0') break;			/* check EOF */
	}
	mcrlf();
}
/* Read messages function. Asks for Next, Previous, a number, etc and processes
for the next msg to be read. Returns 0 if to end reading. Supports the
reply to and Kill function. */

mquery(p,msgnbr,flag)
char *p;
int msgnbr;
int *flag;
{
char c;
int n,killit;
char ln[82],*s;

	cpyarg(ln,p);				/* get possible arg, */
	n= atoi(ln);				/* if valid number, */
	if ((n > 0) && (n <= msg_highest))	/* return it, */
		return(n);

	if (*ln == '*') return(msg_highest);

	line= 0;				/* stop "More?" */

	if (user-> help > REGULAR) mprintf("\r\nRead Message:");
	if (user-> help > EXPERT) {
		mprintf("\r\n1 - %u Reply Enter Kill Prev Next Quit or ? for help",msg_highest);
		if (msg-> reply || msg-> up) mcrlf();
		if (msg-> up) mprintf("+ read the reply  ");
		if (msg-> reply) mprintf("- read the original");
	}
	mprintf("\r\n[%u] 1 - %u R E K P N Q ?: ",msgnbr,msg_highest);

	getstring(ln);
	mcrlf();
	s= skip_delim(ln);

	switch(tolower(*s)) {
		case '-':
			if (msg-> reply) msgnbr= msg-> reply;
			else {
				mprintf("Not a (-) Reply message\r\n");
				msgnbr= -1;
			}
			break;
		case '+':
			if (msg-> up) msgnbr= msg-> up;
			else {
				mprintf("Does not have a (+) Reply\r\n");
				msgnbr= -1;
			}
			break;

		case '?':
			hlpmsg("mquery");
			break;

		case 'p':
			*flag= -1;	/* previous message, */
			goto next;

		case 'n':
			*flag= 1;	/* next highest message */
			goto next;

		case 'q':
			msgnbr= 0;
			break;

		case 'e':
			msend("",0,0); 
			mcrlf();
			goto next;

		case 'r':
			if (strcmp(msg-> from,user-> name) != 0)
				strcpy(ln,msg-> from);
			else if (strcmp(msg-> to,user-> name) != 0)
				strcpy(ln,msg-> to);
			else *ln= '\0';
			msend(ln,msgnbr,0);
			mcrlf();
			if (tolower(s[1]) == 'k') {
				sprintf(ln,"%u",msgnbr);
				mkill(ln);
			}

next:;			if (*flag == 0) *flag= 1;	/* next message, */
			msgnbr += *flag;		/* proper direction, */
			if (msgnbr < 1) {		/* bound it on */
				mprintf("Lowest Message\r\n");
				msgnbr= -1;		/* both ends, */
			} else if (msgnbr > msg_highest) {
				mprintf("Highest Message\r\n");
				*flag= 0;
				msgnbr= -1;
			}
			break;

		case 'k':
			sprintf(ln,"%u",msgnbr);
			mkill(ln);
			mcrlf();
			goto next;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '8':
		case '7':
		case '9':
			msgnbr= atoi(s);
			if ((msgnbr < 1) || (msgnbr > msg_highest)) {
				mprintf("Must be 1 to %u\r\n",msg_highest);
				msgnbr= -1;
			}
			break;
		case '*':
			msgnbr= msg_highest;
			break;

		default:
			goto next;
	}
	return(msgnbr);
}

/* Kill a msg->  The message must be from or to the user, else it cant be 
deleted. If the priv level is 10 or above, then it can be deleted anyways. */

mkill(p)
char *p;
{
int msgnbr;
char work[82];
int flag;	/* needed for numfield */
int reply,up,thisn;

	if (user-> help > EXPERT) mprintf("Delete a message.\r\n");
	sprintf(work,"Kill # (1 to %u)",msg_highest);
	if ((msgnbr= numfield(p,work,&flag)) == 0)
		return;

	if (findmsg(msgnbr,0)) {
		_xclose(msgfile);
		if ((strcmp(msg-> to,user-> name) == 0) || 
		   (strcmp(msg-> from,user-> name) == 0) || (user-> priv >= SYSOP)) {
			up= msg-> up;
			reply= msg-> reply;		/* get pointers, */
			killmsg(msgnbr);		/* delete msg, */
			msg-> reply= 0;			/* (in case not found) */

/* If this message was FROM the caller, and it was not sent, lower the
debit by the cost of the message. */

			if ((strcmp(user-> name,msg-> from) == 0) 
			    && ((msg-> cost > 0)
			    && (msg-> cost <= user-> debit)) ) {
				if (msg-> attr & MSGSENT) {
					mprintf("Already sent, no credit\r\n");
				} else {
					user-> debit-= msg-> cost;
					mprintf("You recovered ");
					dollar(msg-> cost);
					mcrlf();
				}
			}

/* Now fix the reply pointers. If this msg had a reply, change the reply to
point to the deleted msgs reply number. (If 0, no reply) */

			if (up) {			/* if it had a reply, */
				if (thisn= findmsg(up,0)) { /* and correct #, */
					if (msg-> reply == msgnbr) {
						if (reply == thisn) msg-> reply= 0;
						else msg-> reply= reply;
						wrtmsg();
					} else _xclose(msgfile);
				}
			}

/* If this msg was a reply, then change the replied to's msg up pointer to
point to the ext in the chain. (If 0, then no chain.) */

			if (reply) {
				if (thisn= findmsg(reply,0)) {
					if (msg-> up == msgnbr) {
						if (up == thisn) msg-> up= 0;
						else msg-> up= up;
						wrtmsg();
					} else _xclose(msgfile);
				}
			}
			mprintf("Message %u deleted.\r\n",msgnbr);
		} else {
			mprintf("Not your message!\r\n");
			lprintf("Attempted Kill #%u\r\n",msgnbr);
		}
	} else
		mprintf("There is no message #%u.\r\n",msgnbr);
}

/* Convert the passed string into a message number; prompt for one if
the string is blank. Return 0 if bad, else the message number. Returns
cont true if the number ends with a +, for continuous message reading.
If a * is found instead of a number, use the last read msg number plus 1.
If ? is entered, display a help file. */

numfield(p,prompt,flag)
char *p,*prompt;
int *flag;
{
int msgnbr;
char buff[82];
char *s;

	*flag= 0;				/* no plus yet, */
	cpyarg(buff,p);				/* copy possible command tail, */

	while (1) {
		if (strlen(buff) == 0) {		/* if no command tail, */
			mprintf("%s Quit: ",prompt);
			getstring(buff);		/* get the stuff, */
			mcrlf();
			if (cmd_abort(buff)) return(0);	/* abort if blank or Q */
		}
		s= skip_delim(buff);			/* skip blanks, */

		if (*s == '-') {			/* if just -, then */
			*flag= -1;			/* go backwards, */
			return(msg_highest);		/* return highest number, */


/* If nothing special, check for a regular number, possibly trailing + or - */

		} else if ( ((msgnbr= atoi(s)) != 0) && (msgnbr <= msg_highest) ) {
			if (buff[strlen(buff) - 1] == '+') *flag= 1;
			if (buff[strlen(buff) - 1] == '-') *flag= -1;
			return(msgnbr);
		} else mprintf("Must be 1 to %u.\r\n",msg_highest);

		*buff= '\0';		/* force new entry, */
	}
	return(msgnbr);
}
/* Find the specified message number or the next highest one. Return the
message number found, or 0 if none. If the flag is set, it loops, otherwise
it just returns false if the specified one doesnt exist. If a private message,
make sure its the right person or the sysop.
	Any message file found is left open, so that the text can be read if 
necessary. The caller must close it. The one exception is if a msg file is
found, but is private (ie. returned as "not found") then we close it here.
	Loop may be 1 or -1. If 1, then messages are searches from low to high,
else vice versa. */

findmsg(num,loop)
int num,loop;
{
char name[80];

	do {
		if ((num > msg_highest) || (num < 1)) /* check for last msg, */
			return(0);
		sprintf(name,"%s%u.msg",sys-> msgpath,num); /* make filename, */
		msgfile= _xopen(name,2);
		if (msgfile != -1) {
			_xread(msgfile,msg,sizeof(struct _msg)); /* read it in, */
			if (msg-> attr & MSGPRIVATE) {
				if ((user-> priv >= SYSOP) ||
				    (strcmp(user-> name,msg-> to) == 0) ||
				    (strcmp(user-> name,msg-> from) == 0) )
					return(num);
				else _xclose(msgfile);	/* "not found" */
			} else
				return(num);
		}
		num+= loop;	/* increment or decrement, */
	} while (loop);
	return(0);
}
/* List the header of a msg->  */

listhdr(n)
int n;
{
	mprintf("#%-3u %u %s  ",n,msg-> times,msg-> date);
	if (msg-> attr & MSGPRIVATE) mprintf("(PRIVATE) ");
	if (msg-> attr & MSGSENT) mprintf("(SENT) ");
	if (msg-> attr & MSGREAD) mprintf("(RECV'D) ");
	if (msg-> attr & MSGFILE) mprintf("(FILE ATTCHD) ");
	if (msg-> attr & MSGBROAD) mprintf("(BROADCAST) ");
	if (sys-> attrib & SYSMAIL) dollar(msg-> cost);
	mcrlf();

	mprintf("From: %s",msg-> from);
	if ((sys-> attrib & SYSMAIL) && (msg-> orig != 0) && (msg-> orig != -1)) {
		mprintf(" On: Fido #%u, ",msg-> orig);
		if (debug) {
			find_node(msg-> orig);
			mprintf("%s, %s",node.name,node.city);
		}
	}
	mputs("  ");

	mprintf("To: %s",msg-> to);
	if ((sys-> attrib & SYSMAIL) && (msg-> dest != 0) && (msg-> dest != -1)) {
		mprintf(" On: Fido #%u, ",msg-> dest);
		if (debug) {
			find_node(msg-> dest);
			mprintf("%s, %s",node.name,node.city);
		}
	}
	mcrlf();

	if (msg-> reply) {
		mprintf("Reply to msg #%u",msg-> reply);
		if (user-> help > EXPERT) mprintf(" (Use - Key)   ");
		else mprintf("   ");
	}
	if (msg-> up) {
		mprintf("See also msg #%u",msg-> up);
		if (user-> help > EXPERT) mprintf(" (Use + Key)");
	}
	if (msg-> reply || msg-> up) mcrlf();

	if (msg-> attr & MSGFILE) mprintf("File: %s\r\n",msg-> subj);
	else mprintf("Subject: %s\r\n",msg-> subj);
}
/* Kill a message, update the message count, etc. We only have to search
again if we delete the highest one. */

killmsg(num)
int num;
{
char name[80];

	sprintf(name,"%s%d.msg",sys-> msgpath,num);
	_xdelete(name);
	--msg_total;
	if (num == msg_highest) msgcount();
}
/* Count the number of messages and the highest message number, update the
system file array. */

msgcount() 
{
long fsize;
char name[14],spec[80];
int n;

	strcpy(spec,sys-> msgpath);
	strcat(spec,"*.msg");

	msg_total= 0; msg_highest= 0;
	while (_xfind(spec,name,&fsize,0,msg_total)) {
		++msg_total;			/* count another, */
		name[strlen(name) - 4]= '\0';	/* delete the .msg part, */
		n= atoi(name);			/* change name to number */
		if (n > msg_highest) msg_highest= n;/* pick highest one, */
	}
}
/* List all messages addressed to this guy. */

listmsgs() {

#define LMSG 40		/* max msgs to list for each */

int msgnbr,i,direction;
int newmsg[LMSG + 1],oldmsg[LMSG + 1],hismsg[LMSG + 1];

	mprintf("Looking at msg #");
	newmsg[0]= 0;
	oldmsg[0]= 0;			/* clear count and index */
	hismsg[0]= 0;
	msgnbr= msg_highest;
	direction= -1;
	abort= 0;
	while (msgnbr= findmsg(msgnbr,direction)) {
		_xclose(msgfile);
		mprintf("%-3u\010\010\010",msgnbr);
		if ((strcmp(user-> name,msg-> to) == 0) || (msg-> attr & MSGBROAD)) {
			if (msg-> attr & MSGREAD) {
				if (oldmsg[0] < LMSG) oldmsg[++oldmsg[0]]= msgnbr;
			} else {
				if (newmsg[0] < LMSG) newmsg[++newmsg[0]]= msgnbr;
			}
		}
		if (strcmp(user-> name,msg-> from) == 0) {
			if (hismsg[0] < LMSG) hismsg[++hismsg[0]]= msgnbr;
		}
		if (abort) break;
		msgnbr+= direction;
	}
	mcrlf();
	if (abort) mprintf("Searching aborted.\r\n");

	mprintf("New messages to you:\r\n");
	if (newmsg[0] == 0) mprintf(" NONE");
	for (i= 0; i < newmsg[0]; i++) {
		if ((i % 9 == 0) && (i > 0)) mcrlf();
		mprintf("%u, ",newmsg[i + 1]);
	}
	if (newmsg[0] >= LMSG) mprintf("and more! Kill some!");

	mprintf("\r\nOld messages to you:\r\n");
	if (oldmsg[0] == 0) mprintf(" NONE");
	for (i= 0; i < oldmsg[0]; i++) {
		if ((i % 9 == 0) && (i > 0)) mcrlf();
		mprintf("%u, ",oldmsg[i + 1]);
	}
	if (oldmsg[0] >= LMSG) mprintf("and more! Kill some!");

	mprintf("\r\nMessages you have entered:\r\n");
	if (hismsg[0] == 0) mprintf(" NONE");
	for (i= 0; i < hismsg[0]; i++) {
		if ((i % 9 == 0) && (i > 0)) mcrlf();
		mprintf("%u, ",hismsg[i + 1]);
	}
	if (hismsg[0] >= LMSG) mprintf("and more! Kill some!");
	mcrlf();
}
/* Write out the open message. */

wrtmsg() {
	_xseek(msgfile,0L,0);			/* seek to start, */
	_xwrite(msgfile,msg,sizeof(struct _msg)); /* write it out, */
	_xclose(msgfile);
}
/* Return true if the string is blank or contains Q as the first
character. (Abort from command prompts.) */

cmd_abort(s)
char *s;
{
	s= skip_delim(s);
	if ((*s == '\0') || (tolower(*s) == 'q')) return(1);
	return(0);
}
