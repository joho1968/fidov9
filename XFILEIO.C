#include <stdio.h>
#include <ctype.h>
#include <bbs.h>
#include <xtelink.h>

/****************************************************************
*								*
*		       -- X T E L I N K --			*
*								*
*	       File transmit and receive functions		*
*								*
*		   T. Jennings 2 Feb 84				*
*		      created 12 Jan. 83			*
*								*
*								*
****************************************************************/

char *skip_delim();	/* in XENIX.LIB */
char *next_arg();	/* in XENIX.LIB */
char *strip_path();	/* in XENIX.LIB */
char filtchar();	/* in XENIX.LIB */
long _ctol();		/* in XENIX.LIB */

/* Since we dont use the auto-DOS stuff, redefine 
the functions. */

#define xopen _xopen
#define xcreate _xcreate
#define xclose _xclose
#define xread _xread
#define xwrite _xwrite
#define xseek _xseek
#define xfind _xfind
#define ftime _ftime

/* 
;;
;;	FILE: 	FILEIO.C
;;
;;The two functions here, transmit() and receieve() perform the Christensen
;;MODEM7 protocol, XMODEM protocol and the extended TELINK version. They are
;;both called withou args; both prompt for filenames, etc, and display status
;;at the top of the screen dynamically. 
;;
;;There is a global flag, 'filemode', that controls what protocol is used.
;;The (three) possible values are defined in TELINK.H, along with all the
;;static work space, etc.
;;
;;For a good description of the MODEM7 and XMODEM protocols, see the public
;;domain MODEM.DOC. It describes in fair detail how this thing is supposed
;;to work. The TELINK method is described below. Before continuing, however,
;;I should warn you as to the horrible way in which the filename is transferred
;;in MODEM7 moed. It is not documented anywhere; the reason is that it is so 
;;ugly and complicated, with serious bugs that must be accomodated as part
;;of the protocol, it almost can't be defined in any reasonable 
;;fashion.
;;
;;The filename sent consists of 11 upper case characters in CP/M format, that
;;is, 8 character filename, left justified, 3 character extention, left 
;;justified, both fields blank filled. The filename is sent one character
;;at a time, with an odd ack/nak sequence. The first problem is that MODEM7
;;doesn't look at the NAK characters; they must be there however. If an error
;;is encountered, you just send NAKs until MODEM7 receives something like
;;50 of them (you have to look at the MODEM7 source to believe all this)
;;and finally realizes something is wrong. Assuming the name is sent OK,
;;the receiver then send it's checksum back to the transmitter (!) and if
;;OK, the transmitter sends an ACK or whatever. The whole thing is terribly
;;complicated, the only things that makes it useable are (1) there's millions
;;of copies of MODEM7 out there, (2) the sequence is short enough that you
;;don't usually get too many errors.
;;
;;Back to the TELINK protocol: Identical to the MODEM7 protocol, except that
;;before the 1st data block is sent, TELINK slips in a special block that
;contains extra info. Transmission is retried only 4 or 6 times, so that if
;;MODEM7 sees the special block (thinking it's a bad block) it won't exceed
;;the 10 retries of MODEM7. The TELINK block contains:
;;
;;	File size, (long)
;;	Creation time and date (char[4])
;;	ASCIZ filename, (char[16])
;;	TELINK version number (char)
;;	TELINK's own name (char[16])
;;
;;The string "TELINK" is sent over with each file, hopefully if someone
;;rips off TELINK, they'll probably forget this one, and we can catch
;;the bums. Besides, there's lots of extra space.
;;
;;If this block is not received, (transmission error, or MODEM7 transmitting)
;;TELINK operates in MODEM7 mode. In that case, the file size is rounded
;;up to the nearest 128 bytes, the time-to-complete is not displayed, and
;;original file creation time is lost. This is all transparent to the user.
;;
;;
;;These functions are fairly tightly coupled to the screen and keyboard. This
;;may be a pain if it is desired to bury these functions within something else.
;;Basically, all fstat() calls are superfluous; they calculate and display
;;filenames, block numbers, error messages, etc. Some merely report progress;
;;others are reporting errors. There are some counters, totl_ ... that are 
;;used to keep track of soft and hard errors, number of files, etc. They
;;are cleared external to these functions, and could be used for success/
;;failure of a file transmission if all the fstat() stuff were stripped out.
;;Also the chkec_abort() function checks for the user typing ESCape, to abort
;;things. This could merely be removed. If ESC is typed, it sets the error
;;counter to maximum to make the functions stop. ask() is called whenever
;;the error count gets to maximum, to allow manual retries. This, too, could
;;be removed.
;;
;;The top functions, transmit() and receive() are the ones that loop over
;the inputted string, stripping each atom in the string, expanding/removing
;;pathnames, and doing wildcards. All of those string functions are contained
;;in XENIX.LIB. Also there are some kludgey functions, _ctol() and _ltoc()
;;that are used to force longs into the char array and back for transmission
;;over the wire. They also are in XENIX.LIB.
;;
;;
;;PLEASE! If you want to change the stuff in the TELINK statistic block,
;;please add to the end of the array, and bump the version number. DO NOT
;;change the stuff already defined. We may as well keep this compatible. If
;;you need the filename in a different format, add it in, use the version
;;to determine which name to use.
;;
*/

/* Offsets into the data block sent as extended file info: */

#define VERSION 2		/* TELINK extended data block revision, */

#define	OSIZE	0		/* file size, 4 bytes */
#define	OTOD	4		/* creation time and date, 4 bytes, */
#define	ONAME	8		/* file name, 16 bytes */
#define OVERS	24		/* version number (?), 1 byte */
#define OHELLO	25		/* TELINK name */
#define OCRC	39		/* CRC enable flag, */

/* Transmit one or more files. 
In order to handle different disks and paths in batch mode, we seperate
the path or disk from the name, and reappend it to each filename found
in the disk search. */

transmit(cmdstr)
char *cmdstr;
{
int filecount;		/* count of files from expanding input text, */
int filesent;		/* files actually sent to completion, */
char *filespec;		/* ptr to current arg in input line, (linebuf) */
char thisarg[80];	/* current arg copied from filespec (linebuf) */
char name[14];		/* filename found in disk search, */
char prefix[80];	/* path and/or disk spec stripped from filespec */
char work[80];		/* file being sent, prefix plus found filename */
long fsize;		/* required by xfind() */
int i;
int errflg;		/* sucess/failure of file xmission */

	filecount= 0;				/* none found yet, */
	filesent= 0;				/* none sent yet, */
	switch(filemode) {			/* select right mode, */
		case TELINK:
			batflg= 1;		/* batch names, */
			break;
		case MODEM7:
			batflg= 1;		/* filenames, */
			break;
		case XMODEM:
			batflg= 0;		/* no nothing, */
			break;
	}
	statflg= 1;				/* always try stat block, */

	filespec= skip_delim(cmdstr);		/* skip blanks, etc */
	if (num_args(filespec) == 0)		/* if empty line, */
		return(0);			/* return */
	errflg= OK;
	while ((errflg == OK) && (num_args(filespec) > 0)) {
		cpyarg(thisarg,filespec);	/* copy current name, */
		strip_path(prefix,thisarg);	/* seperate disk/path and name */
		i= 0;				/* 0 for 1st xfind, */
		errflg= FERROR;			/* assume bad, */
		while (xfind(thisarg,name,&fsize,0,i)) {
			++filecount;		/* file found, */
			strcpy(work,prefix);	/* assemble the full name, */
			strcat(work,name);

			errflg= sendfile(work,batflg,fsize);
			if (errflg != OK) {
				break;
			}
			++i;			/* next xfind, */
			++filesent;		/* another sent OK, */
		}
		filespec= next_arg(filespec);	/* get next one, */
	}

/* No more files to send. If files were actually sent, send "no more files"
to the remote. */

	if (batflg && filesent) {
		do {i= modin(100);}
		while ((i != TIMEOUT) && (i != NAK));
		if (i == NAK) {
			modout(ACK);		/* name ACK, */
			modout(EOT);		/* no more, */
		}
		modout(EOT);
	}
	flush_mod();				/* flush garbage, */
	return(errflg);
}

/* Get a file or files from the modem. If a wild name is entered, batch mode
is assumed, otherwise just a single file.

As in transmit, the path or disk specifer is stripped off from the input
name, and passed to the getfile function, who appends to the name received
from the remote in batch mode. */
 
recieve(cmdstr)
char *cmdstr;
{
int ret;
int filecount;
char *filespec;		/* ptr to arg in linebuf */
char prefix[80];	/* path or disk removed from input line, (batch only) */

	switch(filemode) {			/* select right mode, */
		case TELINK:
			batflg= 1;		/* batch names, */
			break;
		case MODEM7:
			batflg= 1;		/* filenames, */
			break;
		case XMODEM:
			batflg= 0;		/* no nothing, */
			break;
	}
	statflg= 1;				/* always try statistics, */
	filespec= skip_delim(cmdstr);		/* skip leading spaces, etc */
	if (num_args(filespec) < 1)		/* if empty, return. */
		return(0);
	if (batflg) {
		strip_path(prefix,filespec);	/* seperate disk or path, */
		filecount= 0;
		while ((ret= getfile(prefix,batflg)) == OK) { /* get each file, */
			++filecount;		/* until error, */
		}
	} else {
		ret= getfile(filespec,batflg);	/* single file */
	}
	flush_mod();				/* flush garbage, */
	return(ret);
}		

/* Get a single file from the remote computer. Return ERROR if something
went wrong, SOH if the remote is not in batch mode, or OK if transfer 
complete. If batch mode, the name is a prefix we should stick on the 
beginning of the disk file we create. Does CRC mode if that mode is set,
or a newer TELINK is used, that transmits the CRC flag. */

getfile(name,batch)
char *name;
int batch;
{
int i,filbuf;
int v;
int crctries;
char localname[80];
char work[14];
long fsize;
int bcount;
char filetime[4];
int firstime;

	fsize= 33554432L;		/* big file, 32 M */
	cpyarg(localname,name);		/* init filename, */
	blknum= 0;

/* If batch mode, the filename we're passed is blank, or just a disk or
path spec. Attempt to get one from the remote, if OK, continue. If we 
cant, return an error. */

	flush_mod();			/* empty the line, */

    	errors= 0;
	retrymax= 20;			/* 40 secs max for first time, */

	if (batch) {
		do {	++errors;
			flush_mod();
			firstchar= getfn(work);		/* get a name, */
/**/			if (xferdisp) cprintf("--- Get Filename = %d\r\n",firstchar);
			if (firstchar == OK)		/* if got it, */
				break;			/* get data */
			if (firstchar == EOT)
				return(EOT);
			if (firstchar == ABORT)
				return(ABORT);

		} while (errors < retrymax);
		if (errors >= retrymax) {
			++totl_failures;
			++totl_errors;
			return(ERROR);
		}
		strcat(localname,work);
/**/		if (xferdisp) cprintf("--- Filename = %s\r\n",localname);
	}

/* Create the disk file and display it. Return an error if we cant. */

	if (bad_name(localname)) {	/* dont allow devices etc */
		++totl_failures;
		++totl_errors;
		return(FERROR);
	}

/* If overwriting not allowed, return an error if the file already exists. */

	if (! overwrite) {
		filbuf= xopen(localname,0);
		if (filbuf != -1) {
			xclose(filbuf);
/**/			if (xferdisp) cprintf("--- Already Exists\r\n");
			++totl_errors;
			++totl_failures;
			return(FERROR);
		}
	}

/* Create the new file. */

	filbuf= xcreate(localname,1);
	if (filbuf == ERROR) {
/**/		if (xferdisp) cprintf("--- Cant Create\r\n");
		++totl_failures;
		++totl_errors;
		return(FERROR);
	}

/* Loop here for each block. If the 1st char is a SOH, its a new block; if
it's an EOT, theres no more blocks (close files). Anything else, or a timeout
is an error. If error, falls through to the bottom, count errors, etc.
	To start CRC mode, we send a CRC instead of the initial NAK. We do this
until we receive some character instead of a timeout. If */

	sector= 0;
	blknum= 0;
	errors= 0;
	retrymax= 20;			/* 20 tries (40 secs) for the 1st block, */
	statflg= 0;			/* no time/date yet, */
	firstime= 1;			/* nothing recvd yet */
	crcflag= crcmode;		/* select right mode, */
	ackchar= NAK;
	if (crcflag) ackchar= CRC;	/* use right initial char, */

	do {	++errors;		/* be pessimistic */
/**/		if (xferdisp) cprintf("--- Ack char = %d\r\n",ackchar);
		modout(ackchar);	/* ACK/NAK from previous sector */
		ackchar= NAK;		/* assume error, */
		if (firstime && crcflag) ackchar= CRC;

		firstchar= modin(200);	/* get a char, */

/* If the 1st character is a SYN, and the first sector, its a TELINK info
block containing the file size and file write time and date. This block is 
always sent in checksum mode. SYN blocks are always sent with checksums,
even if the data uses CRCs. Newer TELINKs will transmit the CRC flag;
CRC/checksum is therefore automatic in TELINK mode. */

		if ((firstchar == ABORT) && (sector == 0)) {
			xclose(filbuf);
/**/			if (xferdisp) cprintf("--- ABORT\r\n");
			return(ABORT);

		} else if ((firstchar == SYN) && (sector == 0)) {
			firstime= 0;			/* not syncing anymore */
			ackchar= NAK;			/* assume bad, */
			newsec= modin(100);
			secchk= modin(100);

			if ((newsec == 0) && (secchk == 255)) {
				crcflag= 0;		/* checksum for SYN block, */
				if (getblock() == 0) {
					ackchar= ACK;
					errors= 0;
					fsize= _ctol(&buffer[OSIZE]);
					for (i= 0; i < 4; i++)
						filetime[i]= buffer[OTOD+i];
					statflg= 1;
					crcflag= buffer[OCRC];

				} else crcflag= crcmode; /* else restore mode */
			}
/**/			if (xferdisp) cprintf("--- Telink Block\r\n");

/* If the first character is a SOH, its a data block. */

		} else if (firstchar == SOH) {
			ackchar= NAK;			/* assume bad, */
			firstime= 0;
			newsec= modin(100);		/* sector num */
			secchk= modin(100);		/* 1's compl */
			if (newsec + secchk == 0xff) {	/* if good header */

/* Good block header. Get the data and checksum, if good, ACK it. If its the
same sector number as last time (duplicate block, xmitter didnt get our ACK)
then don't write it to disk. */

				if (getblock() == 0) {

/* If sequential sector number, write it out to disk. */

					if (newsec == ((sector + 1) & 0xff)) {
						errors = 0;
						retrymax= 10;	/* now 10 tries */
						ackchar =ACK;
						++totl_blocks;
						++sector;
						++blknum;
						bcount= (fsize > CPMSIZ) ? CPMSIZ : fsize;
						fsize-= bcount;
						if (bcount > 0) {
							if (xwrite(filbuf,buffer,bcount) != bcount) {
								errors =retrymax;
								modin(1000); /* delay */
								break;
							}
						}

/* If the sector is one we have already received, ignore it, and ACK the sender
to get back in sync. (He probably missed the ACK on that sector.) */

					}  else if (newsec <= sector) {
						errors= 0;
						retrymax= 10;
						ackchar= ACK;

/* Serious problem: We missed a sector. (Probably a NAK from us got garbled
into an ACK on the other end.) Send an ETB as an acknowledge; TELINK will 
backup and re transmit the block. MODEM7 and XMODEM cannot recover from this
error. */
					} else {
						ackchar= NAK;
					}
				}
			} 
/**/			if (xferdisp) cprintf("--- Data Block %u\r\n",blknum);
		} 
		if (errors) {
			++totl_errors;
			while (modin(100) != TIMEOUT);
		}

	} while ((firstchar != EOT) && (errors < retrymax));

	if (statflg)
		ftime(1,filbuf,filetime); /* set file time/date, */
	xclose(filbuf);
/**/	if (xferdisp && (firstchar == EOT)) cprintf("--- EOT\r\n");
	if (firstchar == EOT)
		modout(ACK);			/* ACK the EOT, */
	if ((firstchar == EOT) && (errors < retrymax)) {
		++totl_files;
		return(OK);
	} else {
		++totl_failures;
		return(ERROR);
	}
}
/* Transmit a single file to the remote. Return ERROR if cant. We are passed
a full path name. Attempt to transmit MSDOS extended info: file size, etc
Do only a few attempts, in case the other guy isnt using TELINK. */

sendfile(name,batch,fsize)
char *name;
int batch;
long fsize;
{
char c;
int i,filbuf;
int modesave;
int byte_count;
int ci;
char pathname[80];
char *p;

	blknum =0;
	filbuf= xopen(name,0);		/* open it, */
	if (filbuf == ERROR) {
		++totl_errors;
		++totl_failures;
		return(FERROR);
	}

	if (xferdisp) cprintf("--- File: %s\r\n",name);

/* If batch mode, attempt to send the filename across. If we cant, return
an error. */

	crcflag= 1;			/* just for display, */

	errors =0;			/* reset the error counter, */
	retrymax= 30;			/* 20 tries first time, */
	if (batch) {
		do  {	++errors;			/* count errors */
			p= strip_path(pathname,name);	/* get ptr to name, */
			if (xferdisp) cprintf("--- Sending filename\r\n");
			ci= sendfn(p);			/* if name sent OK, */
			if (ci == OK) break;		/* go send file, */
			if (ci == ABORT) {
				xclose(filbuf);
				return(ABORT);
			}
		} while (errors < retrymax);

		if (errors >= retrymax)	{		/* stop if error */
			xclose(filbuf);			/* close file, */
			++totl_errors;
			++totl_failures;
			return(ERROR);
		}
	}

	errors= 0;				/* wait for initial NAK */
	retrymax= 30;				/* 30 attempts, 60 secs */

/* Wait for the initial NAK or CRC. If we get a NAK, then its checksum mode.
If CRC is received, then CRC mode. Set the flag and display accordingly. */

	if (xferdisp) cprintf("--- Waiting for receiver\r\n");

	while (errors < retrymax) {
		++errors;			/* count an error, */
		i= modin(200);
		if (i == NAK) {			/* if NAK, then checksum method, */
			if (xferdisp) cprintf("--- Received NAK\r\n");
			crcflag= 0;
			break;
		}
		if (i == CRC) {			/* if CRC, the CRC method. */
			if (xferdisp) cprintf("--- Received CRC\r\n");
			crcflag= 1;
			break;
		}
		if (i == ABORT) {
			if (xferdisp) cprintf("ABORT\r\n");
			errors= retrymax;
		}
		if (xferdisp) {
			if (i == TIMEOUT) cprintf("--- TIMEOUT\r\n");
			else cprintf("--- Received %x hex\r\n",i);
		}
	}
	if (errors >= retrymax) {
		++totl_errors;
		++totl_failures;
		xclose(filbuf);
		return(ERROR);
	}

/* If XMODEM, checksum is selected, then dont send the stat block. This is 
because PC-TALK3 has a bad implementation of XMODEM. */

	if ((filemode == XMODEM) && (crcmode == 0)) statflg= 0;

/* Attempt to send the TELINK data block. (In pre v2, was only sent if
TELINK mode.) Do only 4 attempts; if not received by then, assume the other
end is not capable of receiving it. If not received, then the file will
be handled like XMODEM or MODEM7; time and date lost, filesize rounded up
to the nearest 128 bytes. */

	for (i= 0; i < CPMSIZ; i++)
		buffer[i]= 0;		/* clear data block, */

	_ltoc(fsize,&buffer[OSIZE]);	/* install file size, */
	ftime(0,filbuf,&buffer[OTOD]);	/* get file times, */
	p= strip_path(pathname,name);	/* ptr to filename, */
	strcpy(&buffer[ONAME],p);	/* install filename, */
	buffer[OVERS]= VERSION;		/* quickie version number, */
	strcpy(&buffer[OHELLO],"TELINK");
	buffer[OCRC]= crcflag;		/* send the CRC flag, acknowledge it, */

	errors= 0;			/* no errors yet, */
	ci= 0;				/* loop counter, */
	retrymax= 3;			/* max loop/max errors */
	sector= 0;			/* for info block */

	if (statflg) {
		modesave= crcflag;		/* save transfer type, */
		crcflag= 0;			/* do SYN block in checksum, */
		do {
			statflg= 0;		/* assume failure, */
			++ci;			/* loop ctr,NOT 'errors', */
			modout(SYN);
			modout(sector & 0xff);
			modout((~sector) & 0xff);
			send_block();		/* send data, */
			if (modin(700) == ACK) {/* if ACKed, TELINK got */
				statflg= 1;	/* the block, send stats */
				break;		/* next time also, */
			}			++totl_errors;
		} while ((ci < retrymax) && (errors < retrymax));
		crcflag= modesave;		/* restore transfer type, */
	}

/* Now attempt to transmit the data in the file. If not batch mode, this
is the entire file transmission routine. If we get too many errors, ask
to continue, else abort. telling the remote its end of file. */

	sector =1;				/* sector starts at 1, */
	errors= 0;				/* start afresh, */
	retrymax= 20;				/* 20 attempts for 1st blk */

	while (errors < retrymax) {		/* while more chars, */
		byte_count= xread(filbuf,buffer,CPMSIZ); /* fill buffer, */
		if (byte_count == 0) break;	/* go send EOT if no more */

		if (byte_count < CPMSIZ) {	/* fill remainder of */
			for(i= byte_count; i < CPMSIZ; i++)
				buffer[i]= 0x1a; /* block with ^Zs, */
		}

		do {				/* until we get an ACK, */
			if (xferdisp) cprintf("--- Sending block #%u\r\n",blknum);
			modout(SOH);		/* tell rcvr a block comes */
			modout(sector & 0xff);	/* send block #, */
			modout((~sector) & 0xff);/* block check, */
			send_block();

			while (1) {
				ci= modin(1000);/* get acknowledge, */
				if ((ci == ACK) || (ci == NAK) || (ci == TIMEOUT)) break;
			}

			if (ci == ACK) {
				if (xferdisp) cprintf("--- ACK\r\n");
				errors= 0;	/* good ACKnowledge, */
				retrymax= 10;	/* 10 tries from now on */
				++sector;	/* next sector... */
				++blknum;	/* displayed blk num, */
				++totl_blocks;
				break;		/* next sector, */

			} else if (ci == NAK) {
				++errors;	/* another error, */
				if (xferdisp) cprintf("--- NAK\r\n");

			} else if (ci == TIMEOUT) {
				++errors;
				if (xferdisp) cprintf("--- TIMEOUT\r\n");

			} else {		/* ACK/NAK loop timed out */
				errors= retrymax;/* fatal error */
				if (xferdisp) cprintf("--- ACK/NAK timeout recv'd %xh %u\r\n",ci,ci);
			}
		} while (errors < retrymax);
	}

/* All blocks sent, or too many errors. Tell the remote no more. */

	xclose(filbuf);

	if (errors == 0) {
		for (i= 0; i < 5; i++) {
			if (xferdisp) cprintf("--- Sending EOT\r\n");
			modout(EOT);		/* say end of file, */
			if (modin(1000) == ACK)	/* get acknowledgement, */
				break;
			++totl_errors;
		}
	}

	if (errors) {
		++totl_failures;
		return(ERROR);
	} else {
		++totl_files;
		return(OK);
	}
}
/* Receive a 128 byte data block from the modem, calculate the checksum and
the CRC. If CRC mode, check the CRC for correctness, else the checksum. Return
non-zero if wrong. If we get a TIMEOUT, return as a bad CRC or checksum. */

getblock() {
int i,v;

	chksum= 0;			/* initialize check sum and CRC, */
	clrcrc();			/* we maintain both in parallel, */
	for (i= 0; i < CPMSIZ; i++) {
		v= modin(100);
		if (v == TIMEOUT) return(TIMEOUT);
		buffer[i]= v;
		chksum+= v;
		updcrc(v);
	}
	if (crcflag) {			/* if CRC mode, get the two */
		v= modin(100);		/* CRC bytes and do those, */
		if (v == TIMEOUT) return(TIMEOUT);
		updcrc(v);
		v= modin(100);
		if (v == TIMEOUT) return(TIMEOUT);
		updcrc(v);
		return(chkcrc());	/* return result, */
	} else {
		v= modin(100);		/* else do normal checksum, */
		if (v == TIMEOUT) return(TIMEOUT);
		return(v != (chksum & 0xff));	/* return true if not same, */
	}
}
/* Transmit a 128 byte block of data, followed by either the checksum or
CRC, depending on the mode. No return value. (Caller waits for ACK or NAK.) */

send_block()
{

int i;
unsigned crc;

	chksum= 0;			/* maintain both CRC and checksum, */
	clrcrc();
	for (i= 0; i < CPMSIZ; i++) {
		modout(buffer[i]);
		chksum+= buffer[i];
		updcrc(buffer[i]);
	}
	flush_mod();			/* flush out garbage, */
	if (crcflag) {
		crc= fincrc();		/* get CRC bytes, */
		modout(crc >> 8);	/* MS byte first, */
		modout(crc);		/* then LS byte, */
	} else {
		modout(chksum);		/* send checksum, */
	}
}
/* Transmit a filename for batch mode tranmission. Return ERROR if cant do it,
or OK if sent properly. It is assumed that the receiver is ready to receive
the filename we have to send. */

sendfn(name)
char *name;
{
int i;
int cnt;
char localname[14];
char c;
int ci;

	cvt_to_fcb(name,localname);		/* convert name, */

	do ci= modin(100);			/* get initial NAK, */
	while ((ci != NAK) && (ci != TIMEOUT));
	if (ci != NAK)				/* if not NAK, */
		return(ERROR);			/* no filename */

	chksum= 0;				/* start checksum, */
	modout(ACK);				/* all ready, */
	for (i= 0; i < 11; i++) {
		c= localname[i];		/* get name char, */
		chksum+= c;			/* maintain checksum, */
		modout(c);			/* send name char, */
		if ((ci= modin(100)) != ACK)	/* get ack, if bad */
			break;			/* stop sending, */
	}
	if (ci == ACK) {			/* if sent OK, */
		modout(EOFCHAR);		/* end of name, */
		chksum+= EOFCHAR;		/* stupid protocol */
		if (modin(100) == (chksum & 0xff)) { /* get recvr's checksum, */
			modout(OKNMCH);		/* if good, say so, */
			return(OK);		/* return happy, */
		}
	}
	modout(BDNMCH);				/* else not sent OK, */
	return(ERROR);
}
/* Get a filename from the modem, for batch reception. Returns EOT if 
no more files, ERROR if no good, SOH if not batch, or OK. */

getfn(name)
char *name;
{
int i;
int c;			/* NOTE: 'c' is an integer !!! */
char newname[14];

	for (i= 0; i < 11; i++)
		newname[i]= ' ';			/* clear out name, */
	newname[i]= '\0';				/* null terminate, */

	modout(NAK);					/* sync sender, */
	if ((c= modin(200)) != ACK) return(c);		/* return if wrong, */

	chksum= 0;
	for (i= 0; i < 12; i++) {			/* 11 char max, */
		c= modin(100);				/* get a char, */
		if ((c == TIMEOUT) || (c == BDNMCH))	/* if no char recvd, */
			return(ERROR);			/* or bad name, stop. */

		chksum+= c;				/* maintain checksum */
		if (i == 0) {				/* special case 1st */
			if ((c == SOH) || (c == EOT))	/* EOT == no more, */
				return(c);		/* SOH == not batch, */
		}
		if (c == EOFCHAR) {			/* if end of name, */
			modout(chksum & 0xff);		/* send check sum, */
			if (modin(100) == OKNMCH) {
				cvt_from_fcb(newname,name); /* fix name, */
				return(OK);		/* if good, got it. */
			} else {
				return(ERROR);		/* else bad name, */
			}
		}
		newname[i]= c;				/* stash char, */
		modout(ACK);				/* ack reception, */
	}
	return(ERROR);					/* filename too long */
}
/* Convert a CP/M like filename to a normal ASCIZ name. Filter out characters
undersireable on MSDOS. */

cvt_from_fcb(inname,outname)
char *inname,*outname;
{
int i;
char c;

	for (i= 8; i != 0; i--) {		/* do name portion, */
		c= filtchar(*inname++);		/* filter it, */
		if (c != ' ') 			/* if not space, */
			*outname++= c;		/* set it, */
	}					/* do all 8 chars, */
	*outname++= '.';			/* add the dot, */

	for (i= 3; i != 0; i--) {		/* do extention, */
		c= filtchar(*inname++);		/* filter it, */
		if (c == ' ')
			break;
		*outname++= c;
	}
	*outname= '\0';				/* terminate it, */
}
/* Flush the line of any garbage characters. */

flush_mod() {

	while (modin(1) != TIMEOUT);
}
