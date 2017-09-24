#include <stdio.h>
#include <ctype.h>
#include <bbs.h>
#include <xtelink.h>
#include <mail.h>
#include <a:/lib/xfbuf.h>
#include <nodemap.h>
#include <sched.h>

#define REV "a"

/* Define our memory use. File space is used only for guessing
memory requirements. */

unsigned _stack = 32765;		/* for autos and stack, */

main(argc,argv)
int argc;
char **argv;
{
char buff[20],linebuf[82],*p,c;
unsigned v,i;
int interval,redial;
long m,a;
int time;
struct _xfbuf xfbuf;
int do_mail,do_init,disk_full;

	cprintf("Fido's FidoNet  Version %s-%s\r\n",VER,REV);
	cprintf("T. Jennings 1984\r\n");
	allmem();
	m= sizmem();				/* find actual amount, */

	a= (long)_stack;
	a+= (long) (sizeof(struct _usr) * 2);
	a+= (long) sizeof(struct _tlog);	/* all our structures, */
	a+= (long) sizeof(struct _sys);
	a+= (long) sizeof(struct _msg);
	a+= (long) (LINES * COLS);
	a+= (long) _TXSIZE;			/* message, ASCII UL */

	cprintf("%lu bytes of free memory, %lu required\r\n",m,a);
	if (m < a) {
		cprintf("Not enough free memory to run\r\n");
		exit(3);
	}
	cprintf("%lu bytes unused\r\n",m - a);

	p= getml(a);

	user= (struct _usr *)p;		/* make all our structures, */
	p+= sizeof(struct _usr);

	userx= (struct _usr *)p;
	p+= sizeof(struct _usr);

	tlog= (struct _tlog *)p;
	p+= sizeof(struct _tlog);

	sys= (struct _sys *)p;
	p+= sizeof(struct _sys);

	msg= (struct _msg *)p;
	p+= sizeof(struct _msg);

	text= p;				/* message entry */
	p+= (LINES * COLS);

	_txbuf= p;

	*user-> name= '\0';
	user-> priv= NORMAL;
	user-> len= 24;
	user-> width= 80;
	user-> more= 0;
	user-> nulls= 0;

	v= _xopen("system.bbs",0);		/* read system.bbs */
	_xread(v,sys,sizeof(struct _sys));
	_xclose(v);

	newdelim(" \t;,");			/* arg delimiter list, */
	test= 0;				/* default= test off, */
	cd_bit= 0x10;				/* CD bit on Async Card CTS */
	cd_flag= 0;				/* dont ignore CD */
	xferdisp= 1;				/* file xfer display */
	overwrite= 1;				/* allow overwriting files */
	debug= 0;
	do_mail= 1;				/* yes, do outgoing */
	nlimit= 5;				/* 5 connects max */
	bbs= 0;					/* resend messages off, */
	ding= 0;
	redial= 2;				/* max redial */
	disk_full= 0;				/* not yet! */
	do_init= 0;				/* init the modem */
	mdmtype= HAYES;

	while (--argc) {
		strip_switch(buff,*++argv);	/* get the switches, */
		p= buff;
		while (*p) {
			if (*p == 'V') {
				v= atoi(*argv);
				if (v == 0) cprintf("\07Bad CD bit value.\r\n");
				else {
					cd_bit= v;
					cprintf("Modem CD mask= %u\r\n",cd_bit);
				}

			} if (*p == 'T') {
				if (atoi(*argv) > 0) {
					nlimit= atoi(*argv);
					cprintf("%u maximum attempts (with connect)\r\n",nlimit);
				} else cprintf("Bad attempt limit\r\n");

			} if (*p == 'J') {
				switch(atoi(*argv)) {
					case 1:
						mdmtype= HAYES;
						break;

					case 2:
						mdmtype= DF03;
						break;

					case 3:
						mdmtype= VA212;
						break;

					case 0:	/* just /J */
					case 4:
						mdmtype= SMARTCAT;
						break;
				}

			} if (*p == 'U') {
				xferdisp= 0;
				cprintf("No File Transfer Info\r\n");

			} if (*p == 'X') {
				do_mail= 0;
				cprintf("No Outgoing Mail\r\n");

			} if (*p == 'M') {
				bbs= 1;
				cprintf("Test Mode: Resend (SENT) Messages\r\n");

			} if (*p == 'R') {
				do_init= 1;
				cprintf("1200 baud initialization\r\n");

			} if (*p == '?') {
				cprintf("Debug\r\n");
				debug= 1;

			} if (*p == 'L') {
				if (atoi(*argv) > 0) {
					redial= atoi(*argv);
					cprintf("Redial Modulo is %u minutes\r\n",redial);
				} else cprintf("Bad redial modulo\r\n");
			}
			++p;
		}
	}

	cmtfile= _xopen("mailer.log",2);
	if (cmtfile == -1) {
		cmtfile= _xcreate("mailer.log",2);
		if (cmtfile == -1) {
			cprintf("ERROR: mailer.log create error.\r\n");
			exit(1);
		}
	} else {
		_xseek(cmtfile,0L,2);	/* seek to end; append */
	}

	v= _xopen("mail.sys",0);
	if (v == -1) {
		cprintf("Missing MAIL.SYS Cannot do mail\r\n");
		exit(1);
	}
	i= _xread(v,&mail,sizeof(mail));
	if (i < sizeof(mail)) {		/* if pre-v9 mail.sys, */
		strcpy(mail.filepath,mail.mailpath);
	}				/* dup the path name */
	_xclose(v);
	if (mail.node == 0) {		/* if old node number system, */
		mail.node= -1;		/* set to 0 to receive new from fido 1 */
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

/* Initialize the hardware, ave things like BREAK, etc, and generally
get the low level stuff ready for mail. */

	_break(1,0);				/* disable BREAK */
	set_clk();
	init();					/* ready the hardware, */
	wover= 1;				/* no time limit warnings */
	wlimit= 1;

	discon();				/* raise it for the modem, */
	if (do_init) rate= 1;
	else rate= 0;				/* 300 baud for modem init */
	baud(rate);
	cd_flag= 1;				/* ignore carrier detect */
	if (mdmtype != SMARTCAT) 
		init_modem("netmdm.bbs");	/* init the modem, */
	lower_dtr();				/* ignore calls for now, */
	cd_flag= 0;

/* Get the schedule information, and find out which one we are executing. */

	get_sched();				/* load the time table */
	cprintf("This is FidoNet Node #%u\r\n",mail.node);
	event= is_sched('#');			/* find out what schedule we */
	tag= 'A';				/* are running, */
	if (event != -1) {
		tag= sched[event].tag;
		cprintf("Executing Schedule %c\r\n",tag);
	}

/* Count the messages to be sent, and assemble the packets. If none, then
we are to receive mail only. The abort is used if there is a full disk; if
so, set the flag to prevent any mail at all, just wait out the time. */

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
	interval= (gtod3(6) % redial) + 1;	/* initial delay for dial */
	cprintf("Type ^C to abort\r\n\r\n");
	cprintf("(Redial delay is %u minutes)\r\n",interval);
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
				interval= (gtod3(6) % redial) + minutes + 1;
				cprintf("(Redial delay is %u minutes)\r\n",interval - minutes);
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
	reset_clk();
	uninit();
	exit(doscode);
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
