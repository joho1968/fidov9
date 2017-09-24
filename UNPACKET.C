#include <stdio.h>
#include <ctype.h>
#include <bbs.h>
#include <mail.h>
#include <pkthdr.h>

/* Unpack a message packet, put the messages into the mail area. */

unpacket(f)
char *f;
{
int file,n;
struct _hdr hdr;

	file= _xopen(f,0);
	if (f == -1) {
		cprintf("ERROR: Cant open %s\r\n",f);
		lprintf("*Cant open %s\r\n",f);
		logoff(1);
	}

	if (_xread(file,&hdr,sizeof(hdr)) != sizeof(hdr)) {
		cprintf("Incomplete packet %s\r\n",f);
		lprintf("*Incomplete packet %s\r\n",f);
		_xclose(file);
		return;
	}

/* Check to see if our mail. If not, then if the default node number (-1)
and mail from Fido #1, then set the node number. */

	if (hdr.dest != mail.node) {
		if ((mail.node == -1) && (hdr.orig == 1)) {
			n= _xopen("mail.sys",1);
			mail.node= hdr.dest;
			_xwrite(n,&mail,sizeof(mail));
			_xclose(n);
			cprintf("** Received Node Number %u\r\n",mail.node);
			lprintf("** Received Node Number %u\r\n",mail.node);

		} else {
			cprintf("Not our mail! This is to Fido%u\r\n",hdr.dest);
			lprintf("*Packet for Fido%u?\r\n",hdr.dest);
			_xclose(file);
			return;
		}
	}

	cprintf("Packet from Fido%u, ",hdr.orig);
	n= 0;
	while (unp_msg(file,hdr.orig)) ++n;
	_xclose(file);
	cprintf("%u messages\r\n",n);
	lprintf("%u messages in %s\r\n",n,f);
}
/* Unpack a message from the message file. Return 0 if we hit EOF. This 
assembles messages into the BBS message header, then writes them out. */

unp_msg(file,noden)
int file,noden;
{
int msgtype;
char *p;
char msgname[80];
int f,i;

	msgtype= 0;				/* in case read error, */
	_xread(file,&msgtype,sizeof(msgtype));
	switch(msgtype) {
		case 0:
			return(0);
			break;

		case 1:
			break;

		case 2:
			break;

		default:
			cprintf("Message Type %u: Unknown. Aborting packet\r\n",msgtype);
			lprintf("*Message type %u?\r\n",msgtype);
			return(0);
			break;
	}

	msg-> reply= msg-> up= 0;		/* clear replies, etc */
	msg-> dest= mail.node;			/* our node #, */
	msg-> orig= noden;			/* originators node */
	msg-> times= 0;
	for (i= 0; i < sizeof(msg-> caca) / 2; i++)
		msg-> caca[i]= 0;

	msg-> attr= cgeti(file);
	msg-> cost= cgeti(file);
	if (cgets(file,msg-> date) == 0) return(0);
	if (cgets(file,msg-> to) == 0) return(0);
	if (cgets(file,msg-> from) == 0) return(0);
	if (cgets(file,msg-> subj) == 0) return(0);

	p= text;				/* get all the text, */
	while (*p= cgetc(file)) ++p;
	*p= '\0';				/* terminate it, */

	sprintf(msgname,"%s%u.msg",mail.mailpath,msg_highest + 1);
	f= _xcreate(msgname,1);			/* create the message file, */
	if (f == -1) {
		lprintf("*Cant create %s\r\n",msgname);
		cprintf("ERROR: Cant create %s\r\n",msgname);
		logoff(1);
	}
	_xwrite(f,msg,sizeof(struct _msg));
	_xwrite(f,text,strlen(text) + 1);
	_xclose(f);
	++msg_highest;
	++msg_total;
	return(1);
}
/* Read a string from the packet file, including the null. Return 0 if we hit
EOF. */

cgets(f,s)
int f;
char *s;
{

int n;
char c;
	do {
		n= _xread(f,&c,1);
		*s++= c;
	} while (n && c);
	return(n);
}
/* Read an integer from the packet file. */

cgeti(f)
int f;
{
int i;

	_xread(f,&i,sizeof(i));
	return(i);
}
/* Read a character from the packet file. Return null if EOF or null read. */

cgetc(file)
int file;
{
char c;

	if (_xread(file,&c,1) == 0) return('\0');
	else return(c);
}
