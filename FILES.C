#include <stdio.h>
#include <ctype.h>
#include <bbs.h>
#include <xtelink.h>
#include <a:/lib/ascii.h>

extern int seconds,minutes,hours;

/* File system commands. */

/* Search all FILES.BBS's in all directories, and list all matching files. */

ffind(s)
char *s;
{
char line[82],name[82];
int file,found,total,i,m,l;

	if (user-> help > EXPERT) mprintf("Locate a file\r\n");

	if (num_args(s) == 0) {			/* if no arg supplied, */
		mprintf("File to search for: ");
		getstring(line);		/* prompt for it, */
		mcrlf();
		if (num_args(line) == 0) return; /* abort if empty line, */
		s= line;
	}
	cpyarg(name,s);				/* clean copy of filespec, */
	l= pathnum;				/* save orig. path, */
	total= 0;				/* none found yet, */

	for (i= 1; (i <= sysfiles) && !abort; i++) {
		file_dir(i);			/* select and display, */
		if ((strlen(sys-> filepath) > 0) && (user-> priv >= sys-> priv)) {
			total+= file_disp(name,0); /* display all names, */
		}
	}
	mprintf("\r\nFound %u matching files\r\n",total);
	pathnum= l;
	system(0,pathnum);
}
/* Change the default directory. If no arg entered, display DIR.BBS in each
active directory. Get the directory number. */

chdir(n)
int n;
{
int i;
int l,m;
char path[80];

	if (user-> priv >= SYSOP) m= 0;			/* sysops only in #0 */
	else m= 1;
	l= pathnum;					/* save in case no change */

	if ((n < m) || (n > sysfiles)) {
		mprintf("----- File Areas -----\r\n");
		abort= 0;

		for (i= m; i <= sysfiles; i++) {	/* list them all, */
			file_dir(i);
			if (abort) break;
		}
	} else system(0,n);

	while ((n < m) || (n > sysfiles) || (strlen(sys-> filepath) == 0) || (sys-> priv > user-> priv)) {
		mprintf("File Area, or Quit: ");
		getstring(path);			/* get new dir, */
		mcrlf();
		if (cmd_abort(path))			/* if blank no change, */
			n= l;
		else n= atoi(path);			/* get number, */
		if (n <= sysfiles) system(0,n);		/* read that one, */
	} 
	pathnum= n;					/* select permanently, */
	system(0,pathnum);				/* get it. */
}
/* Select the specified system file and display the dir.bbs file 
from it. */

file_dir(n)
int n;
{
char work[80];

	system(0,n);			/* read the system file, */
	if ((sys-> priv > user-> priv) || (strlen(sys-> filepath) == 0)) 
		return;

	stoupper(sys-> filepath);		/* display pathname */
	mprintf("%2u ... %-14s",n,sys-> filepath);
	sprintf(work,"%sdir.bbs",sys-> filepath);
	if (message(work)) mcrlf();	/* CR LF if no dir.bbs */
}
/* Delete a file. NOTE: The filename MUST be in a static (DS) area,
or use the bdos() in LARGE. */

delete(p)
char *p;
{
	p= strip_path(_txbuf,p);	/* remove any path, */
	strcpy(_txbuf,sys-> filepath);	/* assemble the filename, */
	strcat(_txbuf,p);
	bdos(65,_txbuf);		/* delete it */
}
/* Display the downloadable files from the file list. If a filespec is
specified, list only those matching it. In any case, check to see if the
file is really there. Display the actual file size. Return true if any were
found. */

file_disp(s,comments)
char *s;
int comments;
{
char line[82],name[82],pattern[82],fname[82];
int file,found;
long fsize;

	found= 0;			/* none yet, */
	strcpy(line,sys-> filepath);	/* open the files list, */
	strcat(line,"files.bbs");
	file= _xopen(line,0);
	if (file == -1) {		/* if not found, */
		mprintf("No Files\r\n");
		return(found);
	}
	s= strip_path(line,s);
	cpyarg(pattern,s);		/* make the pattern to look for, */
	if (strlen(pattern) == 0) 
		strcpy(pattern,"*.*");	/* if blank, make *.*  */

/* Read lines from FILES.BBS. Pull off the filename (supposed to be in col 0)
and see if it matches. If so, display it, then the file size, (or a msg if
it does not exist) then the rest of the line. Stop if we find aline beginning
with @ */

	while (_xline(file,line,sizeof(line)) && !abort) {
		cpyarg(name,line);			/* get filename, */

		if (*line == 26) break;			/* stop if ^Z */
		if (*line == '@') break;		/* stop if @ sign */

		if ((*line == '-') || (*line == ' ') || (num_args(name) < 1)) {
			if (comments) mprintf("%s\r\n",line);

		} else if (_fmatch(pattern,name)) {	/* see if fname match */
			mprintf("%-13s",name);		/* display the name, */
			strcpy(fname,sys-> filepath);	/* assemble the name, */
			strcat(fname,name);
			if (_xfind(fname,name,&fsize,0,0)) {
				sprintf(name,"%7lu",fsize);
				mputs(name);
				++found;		/* found one, */
			} else {
				mprintf("%7s","MISSING");
			}
			s= next_arg(line);		/* display the rest of */
			mprintf(" %s\r\n",s);		/* the line, */
		}
	}
	_xclose(file);
	return(found);
}

/* Upload or download a file. If not contained in the command tail, prompt
for the download type and the fielname, proceed with the transfer. */

xfer(send,s)
int send;
char *s;
{
char buff[80],filename[80],*p,*q;
char names[400],work[80];
int error,i;
unsigned n;
char mode;
int moresave;
int mins;

	p= s;
	error= 0;
	filemode= -1;
	strcpy(buff,"");

	if (send) mprintf("Download file(s)\r\n");
	else mprintf("Upload file(s)\r\n");

	while (filemode == -1) {
		line= 0;		/* sync up MORE? */
		if ((*p == '\0') || error) {
			mprintf("Transfer Type\r\n");
			if (user-> help > REGULAR) {
				mprintf("A ... Ascii (text only)\r\n");
				mprintf("X ... Xmodem  XC ... Xmodem/CRC\r\n");
				mprintf("M ... Modem7  MC ... Modem7/CRC\r\n");
				mprintf("T ... Telink  TC ... Telink/CRC\r\n");
				mprintf("? ... Help    Q  ... Quit\r\n");
			}
			mprintf("A X XC M MC T TC Q ?: ");
			getstring(buff);
			mcrlf();
			p= skip_delim(buff);
			if (cmd_abort(buff))
				return;
		}
		crcmode= 1;			/* assume CRC, */
		cpyarg(filename,p);		/* get command type, */
		stolower(filename);
		if (strcmp(filename,"?") == 0) {
				hlpmsg("xfertype");
				error= 1;	/* trigger input, */

		} else if (strcmp(filename,"a") == 0) {
				mprintf("ASCII transfer.\r\n");
				filemode= 0;	/* ASCII transfer, */

		} else if (strcmp(filename,"x") == 0) {
				mprintf("XMODEM transfer.\r\n");
				filemode= XMODEM;
				crcmode= 0;

		} else if (strcmp(filename,"xc") == 0) {
				mprintf("XMODEM/CRC transfer.\r\n");
				filemode= XMODEM;
				crcmode= 1;

		} else if (strcmp(filename,"m") == 0) {
				mprintf("MODEM7 transfer.\r\n");
				filemode= MODEM7;
				crcmode= 0;

		} else if (strcmp(filename,"mc") == 0) {
				mprintf("MODEM7/CRC transfer.\r\n");
				filemode= MODEM7;
				crcmode= 1;

		} else if (strcmp(filename,"t") == 0) {
				mprintf("TELINK transfer.\r\n");
				filemode= TELINK;
				crcmode= 0;

		} else if (strcmp(filename,"tc") == 0) {
				mprintf("TELINK/CRC transfer.\r\n");
				filemode= TELINK;
				crcmode= 1;
		} else 
			error= 1;	/* command tail error, */
	}
	p= next_arg(p);

	if ((*p == '\0') || error) {
		mprintf("Filename: ");	/* ask for ang get the */
		getstring(buff);	/* filename, */
		mcrlf();
		p= skip_delim(buff);	/* skip spaces, etc */
		if (strlen(p) == 0)	/* if blank line, return, */
			return;
	}

/* Copy the filenames to the names array. We must do this since for
normal users we dont allow paths and must preprocess. */

	*names= '\0';
	q= p;
	while (num_args(q)) {
		if (!send) strcat(names,sys-> uppath); /* use right path, */
		else strcat(names,sys-> filepath);
		cpyarg(filename,q);		/* user enter filespec, */
		if (bad_name(filename)) {	/* no devices pleez */
			mprintf("%s is not a valid filename\r\n",filename);
			lprintf("* Device Name %s\r\n",filename);
			return;
		}
		strcat(names,strip_path(work,filename)); /* add filename, */
		strcat(names,",");		/* a seperator, */
		q= next_arg(q);			/* skip to next, */
	}

	totl_files= 0;			/* reset file statistics, */
	totl_failures= 0;
	totl_blocks= 0;
	totl_errors= 0;

	if (filemode == 0) {
		moresave= user-> more;	 /* save MORE setting, */
		user-> more= 0;		/* turn off for download */

		if (send) {
			ascii_dl(names);
		} else {
			ascii_ul(names);
		}
		user-> more= moresave; 
		line= 0;
	} else {
		if (send) {
			lprintf("DL %s: ",p);	/* finished below ... */

			if (! exist(names)) {
				mprintf("%s is not there\r\n",p);
				lprintf("N.F.\r\n");
				return;
			}
			mins= xfer_time(names);

			if ((limit > 0) && (mins > (limit - minutes))) {
				mprintf("\07You do not have enough time left.\r\n");
				lprintf(" time limit\r\n");
				return;
			}
			mprintf("Ready to send '%s'\r\n",p); 
			mprintf("Control-C to abort. You have 60 secs. to start.\r\n");
			error= transmit(names);
		} else {
			lprintf("UL %s: ",p);
			mprintf("Ready to receive '%s'.\r\n",p);
			mprintf("Control-C to abort. You have 60 secs. to start.\r\n");
			error= recieve(names);
		}


		switch (error) {
			case ABORT:
				lprintf("OK\r\n");
				mprintf("Aborted.\r\n");
				break;
			case FERROR:
				if (send) {
					lprintf("N.F.\r\n");
					mprintf("File not found.\r\n");
				} else {
					lprintf("Create error\r\n");
					mprintf("Already exists or creation error\r\n");
				}
				break;
			case ERROR:
				lprintf("time out\r\n");
				mprintf("Time out error.\r\n");
				break;
			case OK:
			case EOT:
				lprintf("OK\r\n");
				mprintf("Completed OK.\r\n");
				mcrlf();
				if (!send) descfile(names);
				break;
		}
		mprintf("\07%d files sent OK, %d total data blocks.\r\n",totl_files,
			totl_blocks);
	}
}

/* List disk files. */

dir(string)
char *string;
{
long size,fsize;
int i;
char name[14];
char spec[80];

	strcpy(spec,sys-> filepath);
	if (strlen(string)) 
		strcat(spec,string);
	else 
		strcat(spec,"*.*");

	size= 0L; i= 0;

	while (_xfind(spec,name,&fsize,0,i) && !abort) {
		++i;
		stolower(name);
		mprintf("%14s %10lu\r\n",name,fsize);
		size+= fsize;
	}
	mprintf("%u files, total of %lu bytes.\r\n",i,size);
	xfer_time(spec);
}
/* Return true if the filespec exists. */

exist(spec)
char *spec;
{
long fsize;
char fspec[80],name[14];

	cpyarg(fspec,spec);		/* copy one clean name, */
	return(_xfind(fspec,name,&fsize,0,0));
}
/* Type command. Open the file and type it. Return 0 if file not found or
aborted, else return the number of bytes transferred.  Also, check for whether 
or not this is a text file, by examining the first 1000 bytes. */

ftype(string)
char *string;
{
int file;
int i,s;
unsigned count;
unsigned maxcount;	/* max to read at once */
char c;
int flag;		/* check only first read, */
unsigned alpha;		/* count of alphanumeric, tabs, CR, LF, etc */
unsigned other;		/* all other chars, */
char work[80];

	if (num_args(string) < 1) {
		mprintf("Display (type) a file\r\n");
		mprintf("Filename: ");		/* ask for and get the */
		strcpy(work,"");
		getstring(work);		/* filename, */
		mcrlf();
		string= skip_delim(work);	/* skip spaces, etc */
		if (strlen(string) == 0)	/* if blank line, return, */
			return(0);
	}
	string= strip_path(_txbuf,string);	/* paths not allowed. */
	if (bad_name(string)) {
		mprintf("'%s' is not a valid filename\r\n",string);
		return(0);
	}
	strcpy(_txbuf,sys-> filepath);		/* assemble the filename, */
	cpyarg(&_txbuf[strlen(_txbuf)],string);	/* get the filename, */
	file= _xopen(_txbuf,0);
	if (file == -1) {
		mprintf("Can't find '%s'\r\n",string);
		return(0);
	}

/* Before we attempt to type it, check to see what kind of file this is. By a
rude statistic check, I found that "typeable" files, from pure ASCII like
C and ASM sources, to WordStar files with the 8th bit set, the ratio of
printable (alphas, plus TAB, CR, LF) is 2:1 to the others. There is an
arbitrary lower bound of 100 chars else we dont check. */

	count= 0;
	flag= 0; alpha= 0; other= 0; 
	maxcount= min(1000,_TXSIZE);		/* so 'alpha' and 'other' are small */
	while (s= _xread(file,_txbuf,maxcount)) {
		if ((flag == 0) && (s > 500)) {
			flag= 1;
			for (i= 0; i < s; i++) {
				if ( ((_txbuf[i] > 0x1f) && (_txbuf[i] < 0x7f)) ||
				    (_txbuf[i] == TAB) ||
				    (_txbuf[i] == CR) || 
				    (_txbuf[i] == LF) )
					++alpha;
				else ++other;
			}
			if (alpha < (2 * other)) {
				mprintf("\r\n\07%s is NOT a text file!\r\n",string);
				abort= 1;
			}
		}
		for (i= 0; i < s; i++) {
			if (abort) break;
			c= _txbuf[i];
			if (c == 0x1a) break;
			c &= 0x7f;
			if (c < ' ') switch (c) {
				case CR:
				case LF:
				case TAB:
				case FF:
					mconout(c);
					++count;
					break;
				default:
					break;
			} else {
				mconout(c);
				++count;
			}
		}
		if (abort) break;
	}
	mcrlf();
	_xclose(file);
	return(count);
}
/* Display the transfer time for the file. Return the number of minutes. */

xfer_time(s)
char *s;
{
long size,fsize;
int seconds,mins,secs,bytes_sec;
char name[14],work[80];
int i;

	fsize= 0L;
	while (num_args(s)) {
		i= 0;
		cpyarg(work,s);
		while (_xfind(work,name,&size,0,i)) {
			++i;
			fsize+= size;
		}
		s= next_arg(s);
	}

	switch (rate) {				/* forgot what it is! */
		case 0:				/* 300 baud */
			bytes_sec= 30;		/* 30 chars/sec, */
			break;
		case 1:				/* 1200 baud */
			bytes_sec= 120;
			break;
		case 2:
			bytes_sec= 960;
			break;
		default:
			bytes_sec= 1;
	}
	seconds= fsize/bytes_sec;
	seconds+= (seconds/20);			/* times 1.05, */
	seconds+= (fsize/CPMSIZ)/3;		/* .33 secs blk */
	if (seconds == 0) seconds= 1;
	mins= seconds/60; secs= seconds % 60;
	mprintf("%u total data blocks\r\n",(fsize + 127L) / 128L);
	mprintf("%u min. %u sec. file transfer time.\r\n",mins,secs);
	return(mins);
}
/* Do an ASCII download. */

ascii_dl(names)
char *names;
{
int mins,i,n;
char fname[80];

	cpyarg(fname,names);
	lprintf("ASCII DL %s: ",fname);
	if (! exist(names)) {
		mprintf("%s isn't there\r\n",fname);
		lprintf("N.F.\r\n");
		return;
	}
	mins= xfer_time(fname);
	if ((limit > 0) && (mins > (limit - minutes))) {
		mprintf("\07You do not have enough time left.\r\n");
		lprintf("Time limit\r\n");
		return;
	}
	mprintf("Ready to send '%s', ",fname); 
	mprintf("Control-C aborts, Control-S pauses.\r\n");
	mprintf("When transfer done, you'll get 10 beeps. \r\n");
	mprintf("Strike any key to start ... ");
	mconflush();			/* flush it, */
	mconin();			/* get a key, */
	mcrlf();
	if (n= ftype(fname)) {		/* if ASCII dump OK, */
		mconflush();		/* flush again, */
		for (i= 0; i < 10; i ++) {
			mprintf("\07");
			if (modin(100) != TIMEOUT)
				break;
		}
		totl_blocks= (n + 127) / 128;
		mprintf("ASCII download complete.\r\n");
			lprintf(" OK\r\n");
	} else lprintf(" Aborted or N.F.\r\n");
}
/* Do an ascii upload. Attempt to create the file, and upload to the
text buffer. We assume the guy can support XON/XOFF. */

ascii_ul(names)
char *names;
{
char fname[80];
unsigned size,count,total;
int f,go;
char c;
int echoback;

	cpyarg(fname,names);
	if (bad_name(fname)) {
		mprintf("Not a valid filename\r\n");
		return;
	}

	if (exist(fname)) {
		mprintf("File already exists!\r\n");
		return;
	}
	f= _xcreate(fname,2);
	if (f == -1) {
		mprintf("Cant create the file\r\n");
		return;
	}


	size= _TXSIZE - 2;		/* 2 bytes extra! */
	count= 0;	
	total= 0;
	go= 1;
	echoback= verify("Want your text echoed back to you");
	mprintf("Ready to receive text. When done, type Control-C\r\n");
	mprintf("Control-K, Control-X, ESCape or Delete to stop.\r\n");
	mprintf("Start sending text now.\r\n");

	while (go) {
		c= mconin();
		if (c < ' ') {
			switch(c) {
				case ETX:	/* ^C */
				case CAN:
				case VT:	/* ^K */
				case ESC:	/* ESC */
				case 127:	/* DEL */
					c= '\0';
					go= 0;
					break;

				case CR:	/* CR */
				case LF:	/* LF */
				case TAB:	/* HT */
				case FF:	/* FF */
					break;
				default:
					c= '\0';
					break;
			}
		}

		if (c != '\0') {
			_txbuf[count]= c;
			++count;
			++total;
			if (echoback) mconout(c);
		}
		if (c == CR) {
			_txbuf[count++]= 10;
			if (echoback) mconout(10);
		}
		if (count >= size) break;
	}
	_xwrite(f,_txbuf,count);
	_xclose(f);

	mprintf("\r\nReceived %u characters sucessfully\r\n",total);
	totl_blocks= (total + 127) / 128;
	descfile(fname);
}
