#include <stdio.h>
#include <ctype.h>
#include <bbs.h>
#include <nodemap.h>
#include <mail.h>
#include <sched.h>
#include <a:/lib/xfbuf.h>

/* Main function to support FidoNet. Control is passed here when an
active event is detected, and returns when that event is finished. All
resources are shared, except the log file, which in this case is 
MAILER.LOG. */

do_net() {

int disk_full;
int interval,i;
struct _xfbuf xfbuf;
char linebuf[80];

	overwrite= 1;				/* allow overwriting files */
	disk_full= 0;				/* not yet! */
	wover= 1;				/* no time limit warnings */
	wlimit= 1;

	cmtfile= _xopen("mailer.log",2);
	if (cmtfile == -1) {
		cmtfile= _xcreate("mailer.log",2);
		if (cmtfile == -1) {
			cprintf("ERROR: mailer.log create error.\r\n");
			doscode= 1;
			return;
		}
	} else {
		_xseek(cmtfile,0L,2);	/* seek to end; append */
	}

/* Put some good stuff into the log file. */

	gtod(linebuf);
	lprintf("\r\n---------- FidoNet started at %s FidoNode Fido%u -----------\r\n",
		linebuf,mail.node);

/* Initialize the node map to all blanks. */

	for (i= 0; i < MAXNODES; i++) {
		nmap[i].number= 0;
		nmap[i].tries= 0;
		nmap[i].connects= 0;
		nmap[i].success= 0;
		nmap[i].time= 0;
		nmap[i].msgs= 0;
		nmap[i].files= 0;
	}
	nodetotal= 0;		/* none we know of yet */

	cprintf("This is FidoNet Node #%u\r\n",mail.node);
	cprintf("Executing Schedule %c\r\n",tag);

/* Count the messages to be sent, and assemble the packets. If none, then
we are to receive mail only. The abort is used if there is a full disk; if
so, set the flag to prevent any mail at all, just wait out the time. */

	strcpy(sys-> msgpath,mail.mailpath);	/* for msg stuff */

	msgcount();				/* count all the messages */
	set_abort(0);
	if (was_abort()) {			/* assume disk full, */
		cprintf("(!)       Disk Full! Waiting for Mail Time Over\r\n");
		lprintf("*Disk Full!\r\n");
		disk_full= 1;
		nodetotal= 0;

	} else {
		if (do_mail) make_packets();	/* create packets, etc */
	}

	nn= -1;					/* preincrement at selection time */
	msgfile= 0;				/* NOTE: Used to name in pkts */
	doscode= 0;
	limit= 0;				/* stop time limit, */
	if (!disk_full) raise_dtr();		/* now ready for mail */

	minutes= 0;				/* no elapsed time */
	interval= 1;				/* redial delay */
	cprintf("Type ^C to abort\r\n\r\n");
	cprintf("(0)       Starting Mail Processing\r\n");

/* While in our time window, if we detect a carrier, then receive mail. If
not, delay the random time, then attempt to dial ourselves. When the window is
over, then unpack incoming packets and delete our packets. This is a do ... 
while loop, so that it can be invoked at anytime to handle one call.

If the disk is full, DTR is low, and there are no packets to send. Basically,
nothing happens, except we wait for mail time to end. */

	do {
		if (bdos(6,0xff) == 0x03) {
			doscode= 1;
		}
		if (doscode != 0) break;

		if (dsr()) {
			if (mdmtype == SMARTCAT) init_modem("netmdm.bbs");
			rcv_mail();		/* get mail, */
			discon();		/* disconnect, */
			close_up();		/* close open files, */
			cprintf("\r\n(0)       Starting Mail Processing\r\n");
			interval= 0;		/* dial now */
		}

/* If there are packets waiting to be sent, and the random delay is up,
attempt to send one. If complete, decrement the packet count, and mark
the one just sent as zero. */

		if (interval <= minutes) {
			if (next_node()) {
				send_mail();
				discon();
				close_up();	/* close open files */
				interval= 1;
				cprintf("\r\n(0)       Starting Mail Processing\r\n");
			}
		}

	} while (in_sched(event));

/* Mail time over. Lower DTR, then delete our outgoing packets, and
start processing any packets we received. */

	cprintf("\r\n(*1)      Mail Time is Over\r\n");
	lower_dtr();				/* ignore calls, */

	i= 0;
	xfbuf.s_attrib= 0;
	while (_find("*.out",i,&xfbuf)) {	/* delete working files */
		_xdelete(xfbuf.name);
	}
	i= 0;
	xfbuf.s_attrib= 0;
	while (_find("*.fll",i,&xfbuf)) {	/* delete file lists */
		_xdelete(xfbuf.name);
	}

	i= 0;
	xfbuf.s_attrib= 0;
	while (_find("*.in",i,&xfbuf)) {
		++i;
		cprintf("Unpacking Incoming Packet %s\r\n",xfbuf.name);
		unpacket(xfbuf.name);		/* unpack incoming msgs */
		_xdelete(xfbuf.name);
	}
	if (i == 0) cprintf("No incoming mail\r\n");

/* If we had packets to send, process them now. Mark all sent messages as
SENT, so the sender can know they went out. If any went out, balance all
the accounts in the user records, even if their messages didnt go out. */

	if (nodetotal != 0) {
		cprintf("(*2)      Marking Messages as Sent\r\n");
		cprintf("Node  | tries | cncts | msgs  | files | time  | success\r\n");
		cprintf("------+-------+-------+-------+-------+-------+---------\r\n");
		lprintf("Node  | tries | cncts | msgs  | files | time  | success\r\n");
		lprintf("------+-------+-------+-------+-------+-------+---------\r\n");
		for (i= 0; i <= nodetotal; i++) {
			if (nmap[i].number != 0) {
				cprintf("%5u | %5u | %5u | %5u | %5u | ",
				  nmap[i].number,nmap[i].tries,nmap[i].connects,nmap[i].msgs,nmap[i].files);
				lprintf("%5u | %5u | %5u | %5u | %5u | ",
				  nmap[i].number,nmap[i].tries,nmap[i].connects,nmap[i].msgs,nmap[i].files);

				min_time(nmap[i].time); cprintf("   ");
				lmin_time(nmap[i].time); lprintf("   ");

				if (nmap[i].msgs == 0) {
					cprintf("---\r\n");
					lprintf("---\r\n");

				} else if (nmap[i].success) {
					cprintf("Yes\r\n");
					lprintf("Yes\r\n");
					markmsg(nmap[i].number);

				} else {
					cprintf("No\r\n");
					lprintf("No\r\n");
				}
			}
		}
		cprintf("(*3)      Balancing User Credits\r\n");
		markusr();
		cprintf("(*4)      Marking Broadcasts\r\n");
		markbroad();

	} else cprintf("No outgoing mail\r\n");

	cprintf("(*5)      FidoNet Mail Complete\r\n");
	gtod(linebuf);
	lprintf("------------- FidoNet Closed at %s -------------\r\n",linebuf);
	_xclose(cmtfile);
	return;
}

/* Disconnect. Lower DTR until carrier detect goes away, then raise
it again. */

discon() {

	lower_dtr();				/* drop DTR, */
	cd_flag= 0;
	while (dsr());				/* wait til no carrier, */
	delay(100);				/* delay 1 sec */
	raise_dtr();				/* enable the modem again */
}
/* Select the next node to send mail to. Return true if we find
one, else 0 if no more. */

next_node() {

int i,found;

	if (nodetotal < 1) return(0);		/* obviously none */

	found= 0;				/* pick a node, */
	for (i= 0; i < nodetotal; i++) {
		++nn;
		if (nn > nodetotal) nn= 0;
		if ((nmap[nn].number != 0)	/* if valid number, */
		  && ! nmap[nn].success		/* and not mailed to yet, */
		  && (nmap[nn].connects < nlimit)/* under try limit, */
		  && (nmap[nn].msgs > 0) ) {	/* and has msgs, */
			found= 1;		/*    FOUND ONE! */
			break;
		}
	}
	if (found == 0) return(0);		/* none! */

	find_node(nmap[nn].number);		/* get that node, */
	return(node.number == nmap[nn].number);	/* OK if we found it */
}
/* Since we may have been aborted at any point, we probably
had open files. This is gross, but this closes all possible handles
except the ones we know of (user file, comment file)  */

close_up()
{
int i;
	for (i= 6; i < 20; i++) {
		if (i != cmtfile) _xclose(i);
	}
}
