#include <stdio.h>
#include <ctype.h>
#include <bbs.h>
#include <mail.h>

/* Enter a message. The header (fixed structure in BBS.H) is filled in, then
the message is entered. The message is created in a fixed array, for ease in 
editing. When done, the header is written out, and the text written out as a 
single string.
	The passed arg is used only for the reply-to feature of the read-msg
command; the E command always passes a null. (The input string is not checked
for length or validity.) If a string is passed, the original message number is
stored in the new message.
	If this is a message in the MAIL area, then we also do the destination
system node number. If zero, then this is a local-only message. */

msend(p,replynum,comment)
char *p;		/* who to, */
int replynum;		/* msg # its a reply to, (or 0) */
int comment;		/* true if a comment-to-sysop */
{
char w[82],name[20];
char s1[80],s2[80],*s;
int i,j,ln,width;
int flag,lfound;
int numlines;
int chars;

	width= user-> width;
	if (width > 72) width= 72;

	if (!comment) {
		if (user-> help > EXPERT) mprintf("Enter a message.\r\n");
		mprintf("This will be message #%u.\r\n",msg_highest + 1);
	}
	strcpy(msg-> from,user-> name);
	mprintf("From: %s  On Fido%u\r\n",user-> name,mail.node);

/* Set up in case a msg in the MAIL area. Just returns with LINES if not
the mail area. If 0 lines, then give the no credit message, and terminate.
If a local message, then the destination node number is zero, meaning not
to be forwarded, i.e. local mail. */

	if (replynum) numlines= set_mail(msg-> orig);
	else numlines= set_mail(0);	/* if reply (replynum != 0) then */
	if (numlines < 0) {		/* set dest node to orig node */
		return;
	}
	if (numlines < 1) {
		bbsmsg("nocredit");
		return;
	}
	msg-> dest= node.number;	/* destination node, */
	msg-> orig= mail.node;		/* local system node */

/* In any case, get the person it is To, etc. These are prompted for, unless
this is a reply. */

	if (strlen(p) == 0) {
		getfield(msg-> to,"To: ",0,99,20,1);
		if (strlen(msg-> to) == 0) return;
		if (strcmp(msg-> to,".") == 0) {
			mprintf("Msg to Sysop.\r\n");
			strcpy(msg-> to,"Sysop");
		} 
	} else {
		strcpy(msg-> to,p);
		if (replynum)
			mprintf("REPLY To: %s (#%u)\r\n",msg-> to,replynum);
		else mprintf("To: %s\r\n",msg-> to);
	}

/* Clear out the extra stuff in the message header. Future expansion, and all
that stuff. Also get the subject (handy) and private/non private, etc. */

	msg-> up= 0;		/* clear forward link, */
	msg-> times= 0;		/* hasnt been read yet, */
	msg-> attr= 0;
	msg-> cost= 0;
	gtod(msg-> date);
	for (i= 0; i < (sizeof(msg-> caca) / 2); i++)
		msg-> caca[i]= 0;	/* clear extra space, */
	w[0]= '\0';

/* Allow the attachment of files if SYSOP and command line switch. This puts
the full pathname in the SUBJECT field. NET_ adds this file (if there) after
the message if it exists. If not, then its just a normal message. */

	if ((user-> priv >= SYSOP) && !comment && (sys-> attrib & SYSMAIL)) {
		getfield(w,"Attach a File (y,n): ",0,1,10,1);

		if (*w == 'Y') {
			getfield(msg-> subj,"Full Path Name: ",0,1,72);
			if (strlen(msg-> subj) > 0) {
				msg-> attr |= MSGFILE;
			}
		}
	}
	if (! (msg-> attr & MSGFILE)) {
		getfield(msg-> subj,"Subject: ",0,99,71,0);
		if (strlen(msg-> subj) == 0) return;
	}

	if (!comment) {
		if (user-> priv >= NORMAL) {
			getfield(w,"Private (y,n): ",0,1,10,1);
			if (*w == 'Y') msg-> attr|= MSGPRIVATE;
		}
		if (user-> priv >= SYSOP) {
			getfield(w,"Broadcast (y,n): ",0,1,10,1);
			if (*w == 'Y') msg-> attr|= MSGBROAD;
		}
	} else {
		msg-> attr |= MSGPRIVATE;
	}

/* Main message entry. After a blank line is entered, process commands.  */

	mprintf("Enter your message, blank line to end.\r\n");
	if (user-> help > EXPERT) mprintf("Words will wrap automatically\r\n");

	for (i= 0; i < LINES * COLS; i++)	/* totally clear text */
		text[i]= '\0';

	ln= 0;
	ln= edit(ln,numlines - 1,width - 2);	/* enter text, */

	*w= '\0';
	while (*w != 'S') {
		mprintf("\r\n");
		if (user-> help > REGULAR) mprintf("Enter Msg Command:\r\n");
		if (user-> help > EXPERT)
			mprintf("List Abort To subJect Cont Edit Save ? for help\r\n");
		getfield(w,"L A T J C E S ?: ",1,1,9,1);

		switch(*w) {
			case 'A':
				if (verify("Throw away the message")) {
					mprintf("Message aborted.\r\n");
					return;
				}
				break;

			case 'T':
				if (user-> help > EXPERT)
					mprintf("Type Control-R to retrieve old line.\r\n");
				getfield(msg-> to,"To: ",1,99,20,1);
				break;

			case 'J':
				mprintf("Type Control-R to retrieve old line.\r\n");
				getfield(msg-> subj,"Subject: ",1,99,71,0);
				break;

			case '?':
				hlpmsg("entercmd");
				break;


			case 'C':
				if (user-> help > EXPERT) mprintf("Continue adding to the message\r\n");
				if (ln < numlines) ln= edit(ln,numlines - 1,width - 2);
				else mprintf("Message is full\r\n");
				break;

			case 'L':
				listhdr(msg_highest + 1);
				for (i= 0; i <= ln; i++) {
					mprintf("%2u: ",i + 1);
					s= &text[i * COLS];
					while (*s && !abort)
						mconout(*s++ & 0x7f);
					if (abort) break;
				}
				mprintf("\r\n");
				break;

			case 'E':
				if (user-> help > EXPERT) mprintf("Edit a line\r\n");
				getfield(w,"Line Number: ",0,1,10,0);
				i= atoi(w);
				if (i != 0) {
					if (i <= ln) {
						getfield(s1,"Old String: ",0,10,60,0);
						getfield(s2,"New String: ",0,10,60,0);
						s= &text[(i - 1) * COLS];
						if (substr(s,s1,s2,80) == 0)
							mprintf(" Cannot find [%s]\r\n",s1);
					} else mprintf("Must be Line # 1 to %u\r\n",ln + 1);
				}
				break;

			case 'S':
				mprintf("Saving your message\r\n");
				break;
			default:
				mprintf("'%c' is not a command\r\n",*w);
				break;
		}
	} 

/* Write the header out as is, and convert the message array into one string.
NOTE: Does not save files for TWITs. (Always saves comments.) Count up the
number of characters so we can compute the exact cost of the message later. */

	if ((user-> priv > TWIT) || comment) {
		sprintf(name,"%s%d.msg",sys-> msgpath,msg_highest + 1);
		msgfile= _xcreate(name,1);
		msg-> reply= 0;			/* not yet */
		msg-> up= 0;

/* Count the size of the message. Only really needed for MAIL stuff, but
do it anyways. */

		chars= 0;			/* get size of text of */
		for (i= 0; i <= ln; i++) 	/* the message, */
			chars+= strlen(&text[i * COLS]);

/* If this is the mail area, and the message is to be mailed, and it is not
free, put the cost of this message in the users debit record, to be updated
once the message is mailed. Also put the message cost into the message. */

		if (sys-> attrib & SYSMAIL) {
			mprintf("This message cost you ");
			dollar(cost(chars) + cost(sizeof(struct _msg)));
			mprintf("\r\n");		/* display the cost, */
			user-> debit+= cost(chars) + cost(sizeof(struct _msg));
			msg-> cost= cost(chars) + cost(sizeof(struct _msg));
		}

		_xwrite(msgfile,msg,sizeof(struct _msg));/* write out msg header */

		for(i= 0; i <= ln; i++) {
			s= &text[i * COLS];
			if (_xwrite(msgfile,s,strlen(s)) != strlen(s)) {
				mprintf("\r\n\07DISK FULL: Cannot save your message!\r\n");
				mprintf("Delete some old msgs to make room and start again\r\n");
				lprintf("E MSG: Disk full\r\n");
				if (sys-> attrib & SYSMAIL) {
					mprintf("\r\nYou were not charged for this message\r\n");
					user-> debit-= cost(chars) + cost(sizeof(struct _msg));
				}
				_xclose(msgfile);
				_xdelete(name);
				break;
			}
		}

		_xclose(msgfile);
		++msg_highest;			/* next highest message number, */
		++msg_total;			/* another message */

/* If this is a reply message, find the end of the chain and add the message
there. */

		if (replynum) {				/* if this is a reply, */
			lfound= 0;			/* No "last found" msg, */
			i= findmsg(replynum,0);
/**/			cprintf("- Adding a reply to msg #%u\r\n",replynum);
			while (i) {
				lfound= i;		/* last good msg in chain, */
				if (msg-> up == 0) break;	/* quit if at end, */
/**/				cprintf("- Msg #%u has reply msg #%u\r\n",i,msg-> up);
				if (i == msg-> up) {
					mprintf("Oh no! Message #%u points to itself!\r\n",i);
					break;
				}
				_xclose(msgfile);
				i= findmsg(msg-> up,0);	/* else get child, */
			}

/* Check for the case where we had at least one msg in the chain (lfound) but
one of the children was missing (i == 0). Go back to the last one we had. */

			if ((i == 0) && (lfound != 0)) {
				_xclose(msgfile);	/* may not be open, */
				i= findmsg(lfound,0);	/* open in any case */
			}
			if (i) {
				msg-> up= msg_highest;	/* set fwd ptr in replied-to msg */
				wrtmsg();		/* write it out, */
	
				findmsg(msg_highest,0);	/* the one we just entered */
				msg-> reply= i;		/* set backward ptr */
				wrtmsg();
			} 
		} 
	} else {				/* is TWIT or less */
		for (i= 30000; i > 0; i--);	/* delay, look busy, */
	}
}
/* If in the MAIL area, tell the user how much text they can enter,
what it will cost, and prompt for the destination system. Return the number
of lines they may send (at current width) or 0 if none. If no routing list,
then no mail can be sent; return -1. */

set_mail(noden)
int noden;		/* dest node, for replies */
{
int c,i,n,file;
char *p,line[132],nodename[132];

	n= LINES;				/* in case not mail area */
	make_node("");				/* clear node stuff */

	if (sys-> attrib & SYSMAIL) {

/* List the available nodes, the cost per line to each, and pick one. Costs
are kept in per-cubit; compensate for users line length. */

		mprintf("You have ");
		dollar(user-> credit);
		mprintf(" credit left\r\n");

/* List the available nodes, and select one. If none available, then return
0 lines. */

		if ((noden == 0) || (find_node(noden) == 0)) {
			if (list_nodes() == 0) {
				mprintf("ERROR: nodelist.bbs is missing!\r\n");
				return(-1);
			}
			do {
				getfield(line,"Which System or Quit: ",0,1,10,1);
				make_node("");			/* in case aborted, */
				if (*line == 'Q') break;
				if (*line == '\0') break;
				n= atoi(line);			/* get node #, */
			} while (!find_node(n));		/* get node */

			if (node.number == 0) return(-1);	/* aborted */
		}

/* Given the cost per minute of connect time, the amount of credit left,
calculate the number of (width) long lines max the user can send. */

		if (cost(user-> width) > 0) {
			n= (user-> credit - user-> debit) / cost(user-> width);

		} else n= LINES;

		if (n > LINES) n= LINES;
		mprintf("You have enough credit left for %u lines\r\n",n);
	}
	return(n);
}
