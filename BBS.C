#include <stdio.h>
#include <ctype.h>
#include <bbs.h>
#include <mail.h>

/* Log off the system. Put logoff stuff in the log files, disconnect, and
force termination with a frc_abort() to return to  the main. */

logoff(code)
int code;
{
char buff[20];
int f;

	if (mode == FIDO) {
		if (userflag) {
			f= _xopen("user.bbs",2);
			if (f == -1) {
				mprintf("ERROR: No user list!\r\n");
				code= 1;

			} else {
			user-> time+= minutes;		/* accumulate time on, */
				user-> msg= msgnum;		/* save last selected */
				user-> files= pathnum;		/* directories, */
				strcpy(user-> ldate,date);	/* last time called, */
				_xseek(f,userpos,0);		/* proper place, */
				_xwrite(f,user,sizeof(struct _usr)); /* update, */
				_xclose(f);
			}
			sysnum= -1;				/* ??? force reading of file */
			system(0,0);				/* read main system file, */
			++sys-> caller;				/* update caller number, */
			system(1,0);				/* write back out, */
		}
		if (cmtflag) {
			lprintf("\r\nAccum DL %uK, Total DL %uK, Total UL %uK\r\n",
				user-> dnldl,user-> dnld,user-> upld);
			lprintf("Accum. time %u min\r\n",user-> time);
			gtod(buff);
			lprintf("%s off at %s.\r\n",user-> name,buff);
		}
	}
	if (test) doscode= 1;
	frc_abort();
}

/* Change the user record. */

change(s)
char *s;
{

char buf[10],ans[20],work[80];
int i,f;

	do {
		line= 0;
		mprintf("----- Change User -----\r\n");
		user_credit();
		mprintf("User name : %s\r\n",user-> name);
		mprintf("City, ST  : %s\r\n",user-> city);
		mprintf("Password  : %s\r\n",user-> pwd);
		mprintf("Help Level: ");
		 switch(user-> help) {
			case 0:		/* old pre v7 levels */
			case 1:
				user-> help= NOVICE;
			case NOVICE:
				mprintf("NOVICE\r\n");
				break;
			case REGULAR:
				mprintf("REGULAR\r\n");
				break;
			case EXPERT:
				mprintf("EXPERT\r\n");
				break;
		 }
		mprintf("----- Your Screen -----\r\n");
		mprintf("Nulls     : %u\r\n",user-> nulls);
		mprintf("\"More?\"   : %s\r\n", (user-> more ? "ON" : "OFF"));
		mprintf("Width     : %u columns\r\n",user-> width);
		mprintf("Length    : %u lines\r\n",user-> len);
		mprintf("Tabs      : %s\r\n",(user-> tabs ? "ON" : "OFF"));		
		if (user-> help > REGULAR)
			mprintf("Change:\r\n");
		if (user-> help > EXPERT)
			mprintf("User-Name City Password Help-Level\r\n");
			mprintf("Nulls More Width Length Tabs Quit ?\r\n");
		getfield(buf,"U C P H N M W L T Q ?: ",0,1,10,1);

		if (cmd_abort(buf)) break;	/* terminate if Q or blank */

		switch(*buf) {
		case 'H':
			i= 0;
			while (i == 0) {
				mprintf("----- Set Help Level -----\r\n");
				mprintf("1 ... No Help. Expert.\r\n");
				mprintf("2 ... Some Help. Experienced.\r\n");
				mprintf("3 ... Full Help. Novice.\r\n");
				getfield(buf,"Pick One: ",0,1,10,1);
				switch(*buf) {
					case 'N':
					case '1':
						i= EXPERT;
						break;
					case 'S':
					case '2':
						i= REGULAR;
						break;
					case 'F':
					case '3':
						i= NOVICE;
						break;
					case 'Q':
					case '\0':
						i= user-> help;
						break;
				}
			}
			user-> help= i;
			break;

		case 'U':
			getfield(work,"New name, 1 or 2 words: ",0,2,36,1);
			if (strlen(work) == 0) break;
			if (strcmp(work,user-> name) == 0) break;

			mprintf("Checking the users list, please wait ...");
			i= 0;			/* name not found yet, */
			f= _xopen("user.bbs",0);
			if (f == -1) {
				mprintf("ERROR: User list is bad\r\n");
				doscode= 1;
				frc_abort();
			}
			while (_xread(f,userx,sizeof(struct _usr)) == sizeof(struct _usr)) {
				if (strcmp(work,userx-> name) == 0) {
					i= 1;		/* found name, */
					break;
				}
			}
			_xclose(f);
			if (i) mprintf("\r\nThat name already exists. Choose another.\r\n");
			else {
				mprintf("\r\nNew name is OK\r\n");
				strcpy(user-> name,work);
			}
			break;
		case 'P':
			getfield(user-> pwd,"New password, 1 word: ",1,1,14,1);
			break;
		case 'C':
			getfield(user-> city,"City and two letter state: ",0,3,35,1);
			break;
		case 'N':
			getfield(ans,"Number of nulls (0 - 20): ",0,1,10,1);
			if (cmd_abort(ans)) break;
			i= atoi(ans);
			if ((i > 20) || (i < 0))
				mprintf("Must be 0 to 20.\r\n");
			else
				user-> nulls= i;
			break;
		case 'L':
			getfield(ans,"Lines on Screen (16 - 66): ",0,1,10,1);
			if (cmd_abort(ans)) break;
			i= atoi(ans);
			if ((i < 16) || (i > 66))
				mprintf("Must be 16 to 66\r\n");
			else
				user-> len= i;
			break;
		case 'W':
			getfield(ans,"Screen Width (20 - 80): ",0,1,10,1);
			if (cmd_abort(ans)) break;
			i= atoi(ans);
			if ((i < 20) || (i > 80))
				mprintf("Must be 20 to 80\r\n");
			else
				user-> width= i;
			break;

		case 'T':
			user-> tabs^= 1;	/* toggle */
			break;
		case 'M':
			user-> more^= 1;	/* toggle */
			break;
		case '?':
			hlpmsg("c");
			break;
		}
	} while (*buf != '\0');
}
/* List all the users on the system. */

users(p)
char *p;
{
int f;
	f= _xopen("user.bbs",0);
	if (f == -1) {
		mprintf("ERROR: Cannot find the user file!\r\n");
		doscode= 1;
		frc_abort();
	}
	line= 0;
	abort= 0;
	while (!abort) {
		if (line == 0) 
			mprintf("%-30s %-20s %s\r\n","Name","Last called","City");
		if (_xread(f,userx,sizeof(struct _usr)) != sizeof(struct _usr)) 
			break;
		mprintf("%-30s %-20s %s\r\n",userx-> name,userx-> ldate,userx-> city);
	}
	_xclose(f);
	mcrlf();
}

/* Sysop command. Test modem timeout. */

mtest(p)
char *p;
{
int i;
	mprintf("Strike any key");
	mconin();
	mcrlf();
	for (i= 0; i < 10; i++) {
		modin(100);
		mprintf("%d ",i + 1);
	}
	mprintf("\07");
}
/* System operator command. Change the path(s) in the system file. Obviously
to be used with great care. */

spath(p,path)
char *p,*path;
{
int n;
char s[80];

	strcpy(s,path);				/* strip off possible */
	if (strlen(s) > 0) {			/* trailing / from it */
		s[strlen(s) - 1]= '\0';
	}

	if (isdigit(*p)) {
		n= atoi(p);
		if (n > sysfiles + 1)
			mprintf("Illegal system file number!\r\n");
		else {
			if (n == sysfiles + 1) mprintf("New system file\r\n");
			system(0,n);		/* select or create system file, */
		}
		msgnum= n;			/* force selection, */
		pathnum= n;
		mprintf("Wait ...\r\n");
		syscount();			/* recount the system files, */
		msgcount();			/* recount messages */
	} 
	mprintf("System file #%u\r\n",sysnum);	/* say which one, */
	if (*p == 'm')
		strcpy(sys-> msgpath,path);
	else if (*p == 'f')
		strcpy(sys-> filepath,path);
	else if (*p == 'b')
		strcpy(sys-> bbspath,path);
	else if (*p == 'h')
		strcpy(sys-> hlppath,path);
	else if (*p == 'm')
		strcpy(sys-> msgpath,path);
	else if (*p == 'u')
		strcpy(sys-> uppath,path);
	else if (*p == 'r')			/* restore original contents */
		system(0,sysnum);
	else if (*p == 's')
		system(1,sysnum);		/* write out new contents, */
	else if (*p == 'v')
		setpriv(s,&sys-> priv);		/* set min. privelege */
	else if (*p == 'a')
		sys-> attrib= atoi(s);		/* set attribute  */

/*	else if (*p == '?') {
		mprintf("1 <path>\\F, B, M, H for Files, BBS, Msg, or Help paths,\r\n");
		mprintf("1 <number> to change or create a new system file,\r\n");
		mprintf("1 <priv>\\V to set the privelege level, where <priv> is\r\n");
		mprintf("1 <number>\\A to set attributes. Current attributes are:\r\n");
		mprintf("  1 == MAIL area. Use 0 to clear.\r\n");
		mprintf("1 S to Save the new contents of this system file, \r\n");
		mprintf("1 R to Restore the original contents of this system file.\r\n");
		mprintf(" TWIT, DISGRACE, NORMAL, PRIVEL, EXTRA, SYSOP\r\n");
		mcrlf();
	} */ else {
		mprintf("Minimum Privelege: ");
		getpriv(sys-> priv);
		mprintf("\r\nBBS  path: %s\r\nMSG  path: %s",
			sys-> bbspath,sys-> msgpath);
		if (sys-> attrib & SYSMAIL) mprintf(" MAIL Section\r\n");
		else mprintf(" MSG Section\r\n");
		mprintf("HELP path: %s\r\nFILE path: %s\r\nUPLD path: %s\r\n",
			sys-> hlppath,sys-> filepath,sys-> uppath);
	}
}
/* Display and change the MAIL.SYS file. */

setmail(p,path)
char *p,*path;
{
float f;
int n;
char s[80];

	switch(*p) {
		case 'p':
			strcpy(mail.mailpath,path);
			break;

		case 'f':
			strcpy(mail.filepath,path);
			break;

		case 'n':
			mail.node= atoi(path);
			break;

		case '?':
			mprintf("FidoNet node parameters:\r\n");
			mprintf("4 <number\\N  sets this node number (1 - 32767)\r\n");
			mprintf("4 <path>\\P sets the mail path\r\n");
			mprintf("4 <path>\\F sets the mail file path\r\n");
			break;
	}
	mprintf("\r\nFidoNet Parameters:\r\n");
	mprintf("FidoNet Node       : FIDO%u\r\n",mail.node);
/*	sprintf(s,"Cost Fudge Factor  : %f\r\n",mail.fudge); 
	mputs(s);
*/	mprintf("Mail Directory     : %s\r\n",mail.mailpath);
	mprintf("Mail File Directory: %s\r\n",mail.filepath);
	strcpy(s,sys-> bbspath);
	strcat(s,"mail.sys");
	n= _xopen(s,2);
	_xwrite(n,&mail,sizeof(mail));
	_xclose(n);
}

/* List the free disk space. */

freespace()
{

unsigned i[4];
char f[80];
long n1,n2;

	_free(0,i);			/* get disk stuff, */
	n1= i[3]; n1*= i[2]; n1*= i[0];	/* total clust * bytes/sec * secs/clust */
	n2= i[1]; n2*= i[2]; n2*= i[0];	/* free clust * bytes/sec * secs/clust */

	mprintf("%lu bytes total disk space, %lu free.\r\n",n1,n2);
}

/* List the users statistics. */

stats() {

char tod[20];
int kleft,dleft,tleft,lim;

	gtod(tod);
	mprintf("%s, on for ",tod);
	min_time((minutes * 60) + seconds);
	mprintf(" mins.\r\nYour "); nth(user-> times);mprintf(" call\r\n");

	tleft= limit - minutes; if (tleft < 0) tleft= 0;
	dleft= dlimit - user-> time; if (dleft < 0) dleft= 0;
	kleft= klimit - user-> dnldl; if (kleft < 0) kleft= 0;

	mprintf("            | Total Today Limit  Left\r\n");
	mprintf("------------+------------------------\r\n");
	mprintf("Downloaded: | %4u  %4u  %4u  %4u Kbytes\r\n",user-> dnld,user-> dnldl,klimit,kleft);
	mprintf("Uploaded:   | %4u                   Kbytes\r\n",user-> upld);
	mprintf("This call:  | %4u        %4u  %4u Min.\r\n",minutes,limit,tleft);
	mprintf("Per 48 Hrs: |       %4u  %4u  %4u Min.\r\n",user-> time,dlimit,dleft);
}
/* User logoff command. */

goodbye(p)
char *p;
{
int n;
char work[80];

	p= skip_delim(p);
	cpyarg(work,p);
	if (! *p) {
		mprintf("Leave a private message to the sysop? (y,n): ");	
		getstring(work);
		mcrlf();
	} 
	if (tolower(*work) == 'y') {
		n= msgnum;		/* save  msg path, */
		msgnum= 0;
		system(0,msgnum);	/* put in area 0, */
		mprintf("Wait ...\r\n");
		msgcount();
		msend("Sysop",0,1);	/* get a message */
		msgnum= n;
		system(0,msgnum);	/* reselect orig. area */
	}
	gtod(work);
	mprintf("Logging %s off at %s. Hang up now.\r\n",user-> name,work);
	logoff(0);
}
/* Display the users credit and debits. */

user_credit() {

	mprintf("You have ");
	dollar(user-> credit); mprintf(" credit, "); 
	dollar(user-> debit); mprintf(" charges pending.\r\n");
}
