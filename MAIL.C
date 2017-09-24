#include <stdio.h>
#include <ctype.h>
#include <bbs.h>
#include <xtelink.h>
#include <mail.h>
#include <a:/lib/xfbuf.h>
#include <nodemap.h>

#define TSYNC 0xae

/* Carrier detected. Determine the baud rate, display a message to
tell humans what were doing, then get in sync with the sender. Once
in sync, receive the mail packet. */

rcv_mail() {

int i,n;
char pktname[80];
int msg_pkts,msg_blocks;

	gtod(pktname);
	lprintf("Call in @ %s\r\n",pktname);	/* log the call, */

	cprintf("(1)       Carrier Detected\r\n");
	clr_clk();			/* reset the clock, */
	limit= 1;			/* limit to connect, */
	cd_flag= 0;			/* watch Carrier Detect */

/* We get an abort if: timeout, no sync (human caller), actual file transfer
error, or packet received and sender hangs up. Totl_files is 0 if no
packets transferred. */

	totl_blocks= 0;
	totl_files= 0;
	msg_pkts= 0;
	msg_blocks= 0;
	set_abort(0);			/* trap carrier loss here, */
	if (was_abort()) {
		if (msg_pkts == 0) {
			cprintf("          Receiving Mail Packet Aborted\r\n");
			lprintf("\r\n*Aborted\r\n");

		} else {
			cprintf("(5)       Received %u packets\r\n",msg_pkts);
			lprintf(" %u pkts %u blks OK\r\n",msg_pkts,msg_blocks);
			if (totl_files > 0) {
				cprintf("          Received %u files\r\n",totl_files);
				lprintf("Recv'd Files\r\n");
			}
		}
		cd_flag= 1;			/* ignore CD, */
		delay(200);			/* delay for telco */
		return(msg_pkts);
	}

/* The sender whacks CR until it gets a CR back. It then waits for the
message to stop, sends a TSYNC, and starts the transfer. */

/* -------------------- One Minute to Complete This ---------------------- */

	cprintf("(2)       Determining Baud Rate\r\n");
	connect();			/* determine baud rate, */
	modout(CR); 			/* tell Tx we got baud right */

	mprintf("\r\n   FidoNet Node #%u\r\n",mail.node);
	mprintf("   Processing mail only.\r\n");
	mcrlf();

	cprintf("(3)       Waiting for Sync\r\n");
	while (modin(100) != TSYNC)
		limitchk();

/* ------------------- End One Minute to Complete -------------------- */

/* All connected. Transfer the mail packet. No time limit, but the
file receive will abort if there is an error. */

	limit= 0;			/* no time limit */

	sprintf(pktname,"%u.in",msgfile++);	/* pick a new name, */
	cprintf("(4)       Waiting for Mail Packet (%s)\r\n",pktname);
	lprintf(" %s ",pktname);

	crcmode= 1;			/* CRC, */
	filemode= XMODEM;		/* file xfer mode, */
	recieve(pktname);		/* receive a packet, */

	msg_pkts= totl_files;
	msg_blocks= totl_blocks;	/* number of mail packets, */
	totl_files= 0;
	totl_blocks= 0;

/* Let extra EOTs, etc settle out, then flush it so the file receive
doesnt get the extra EOTs. */

	delay(200);			/* delay for last char */
	while (modin(10) != TIMEOUT);

/* Wait for any incoming files. If there areent any, then the caller
will just hang up if an older version, or will send EOT if no files. */

	filemode= TELINK;
	recieve(mail.filepath);		/* get any files, */
	delay(200);
	frc_abort();			/* error trap exit */
}

/* Send a mail packet to the remote. Dial the number, if error, return 0 to
indicate no connection. Also, 'minutes' upon return reflects the actual
connection time. */

send_mail() {

char pktname[80],fllname[80];
int n,f;
int msg_pkts,msg_blocks;

	cprintf("(1)       Calling Fido%u %s %s\r\n",node.number,node.name,node.phone);

	*_txbuf= '\0';			/* in case no file, */
	sprintf(fllname,"%u.fll",node.number);
	f= _xopen(fllname,0);		/* read in file list, */
	n= _xread(f,_txbuf,_TXSIZE);
	_txbuf[n]= '\0';		/* null terminate it, */
	_xclose(f);

	gtod(pktname);
	lprintf("Call out Fido%u @ %s\r\n",node.number,pktname);

	sprintf(pktname,"%u.out",node.number);	/* local packet to send */
	++nmap[nn].tries;		/* log another try, */

/* We get an abort if (1) it times out dialing, (2) we fail to connect 
or (3) transmission sucessful, and receiver hung up. totl_files tells
us what happened.(NOTE: Dialing timeout is indicated by cd_flag being set;
if it is, no connection was made, do not count the time.) */

	totl_blocks= 0;
	totl_files= 0;
	msg_pkts= 0;
	msg_blocks= 0;
	set_abort(0);
	if (was_abort()) {
		if (cd_flag == 0) {
			nmap[nn].time+= (minutes * 60) + seconds;
			++nmap[nn].connects;
		}
		if (msg_pkts == 0) {
			cprintf("          Packet Transmission Aborted\r\n");
			lprintf("*Nothing sent\r\n");

		} else {
			cprintf("(5)       Sent %u packets %u files\r\n",
				msg_pkts,totl_files);

			lprintf(" Sent %u pkts %u files\r\n",msg_pkts,totl_files);
			nmap[nn].success= 1;
			_xdelete(pktname);	/* done with that packet */
			_xdelete(fllname);
		}
		return(msg_pkts);
	}

/* Set the baud rate from the node list, and dial the number. Dialing is
aborted if either a no-connect from the modem, or a timeout if no modem
is connected. */

	rate= 0;				/* assume 300 baud, */
	if (node.rate >= 1200) rate= 1;		/* unless higher */
	baud(rate);
	cd_flag= 1;				/* dont bomb on no carrier */

/* -------------------- One Minute to Dial ---------------------- */

	limit= 1;				/* 1 min to dial, */
	clr_clk();				/* reset the clock, */
	if (dial(node.phone) == 0) {
		cprintf("(2)       No connection\r\n");
		lprintf("*No connection\r\n");
		frc_abort();			/* do abort handler */
	}
	rate= 0;				/* again for stupid hardware */
	if (node.rate >= 1200) rate= 1;		/* that dials at fixed rates */
	baud(rate);

	cd_flag= 0;				/* now watch carrier */
	cprintf("(2)       Connected at %u baud\r\n",node.rate);
	lprintf(" Connect @ %u\r\n",node.rate);

/* -------------------- One Minute to Connect and Sync --------------- */

/* Send CRs until we get a CR back, saying we detected baud correctly. */

	clr_clk();				/* reset clock, */
	limit= 1;				/* 1 min to connect, etc */

	while (modin(300) != TIMEOUT)		/* quiet line, */
		limitchk();

	for (n= 8; --n;) {			/* limited tries for baud test */
		modout(CR);			/* send a CR until we */
		if (modin(100) == CR) break;	/* get one back, */
	}
	if (n == 0) {
		lprintf("*autobaud failed\r\n");
		frc_abort();			/* give up if no response */
	}
/* We got a CR from the receiver. Baud rate detected. Now, wait for the
prompt junk to go away, then send our sync character to say we're ready
to send. */

	while (modin(100) != TIMEOUT)		/* wait for quiet line, */
		limitchk();
	modout(TSYNC);				/* send our sync, */
	lprintf(" Sent TSYNC\r\n");

/* -------------------- End One Minute Connect and Sync -------------- */

	limit= 0;				/* no limit now */

	cprintf("(4)       Sending %s\r\n",pktname);

	filemode= XMODEM;
	crcmode= 1;
	transmit(pktname);
	msg_pkts= totl_files;
	msg_blocks= totl_blocks;
	totl_files= 0;
	totl_blocks= 0;
	if (strlen(_txbuf) == 0) {		/* if no files, just */
		modout(EOT);			/* send EOT, */

	} else {
		filemode= TELINK;		/* now send any files, */
		transmit(_txbuf);
	}
	delay(500);
	frc_abort();				/* do termination code */
}
