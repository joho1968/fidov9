#include <stdio.h>
#include <ctype.h>

/* Define the first and last bytes, for static mem size determination */

char f_byte;		/* first byte */
#include <bbs.h>
#include <xtelink.h>	/* includes in between */
#include <mail.h>
#include <sched.h>
#include <nodemap.h>
char l_byte;		/* last byte */

/* BBS main functions. See BBS.DOC */

unsigned _stack = 40000;		/* for autos and stack, */

main(argc,argv)
int argc;
char **argv;
{
char buff[20],linebuf[82],*p,c;
unsigned v,i,f;
long m,a;
int _dlimit,_klimit;

	cprintf("Fido and FidoNet System Version %s T. Jennings 1983\r\n",VER);
	allmem();
	m= sizmem();				/* find actual amount, */

	a= (long) _stack;
	a+= (long) (sizeof(struct _usr) * 2);
	a+= (long) sizeof(struct _sys);
	a+= (long) sizeof(struct _msg);
	a+= (long) (LINES * COLS);
	a+= (long) _TXSIZE;			/* message, ASCII UL */

	cprintf(" - %6lu byte static space\r\n",(long) (&l_byte - &f_byte));
	cprintf(" - %6lu byte heap space\r\n",a);
	cprintf(" - %6lu byte stack space\r\n",(long) _stack);
	cprintf(" - %6lu byte heap size\r\n",m);
	if (m < a) {
		cprintf("Not enough free memory to run\r\n");
		exit(3);
	}
	cprintf(" - %6lu bytes leftover\r\n",m - a);

	p= getml(a);

	user= (struct _usr *)p;		/* make all our structures, */
	p+= sizeof(struct _usr);

	userx= (struct _usr *)p;
	p+= sizeof(struct _usr);

	sys= (struct _sys *)p;
	p+= sizeof(struct _sys);

	msg= (struct _msg *)p;
	p+= sizeof(struct _msg);

	text= p;				/* message entry */
	p+= (LINES * COLS);

	_txbuf= p;

	sys-> caller= 0;
	sys-> priv= DISGRACE;
	*sys-> bbspath= '\0';
	*sys-> msgpath= '\0';
	*sys-> hlppath= '\0';
	*sys-> filepath= '\0';
	*sys-> uppath= '\0';

	newdelim(" \t;,");			/* arg delimiter list, */
	test= 0;				/* default= test off, */
	logflag= 0;				/* logging= off, */
	cmtflag= 0;				/* comments= off */
	userflag= 0;				/* user file not open, */
	msgfile= -1;				/* message file not open, */
	bbs= 1;					/* BBS system = true, */
	ding= 1;				/* allow bells to console, */
	debug= 0;				/* no debug features */
	slimit= 10;				/* default 10 signon limit */
	nlimit= 60;				/* default= 1 hr time limit, */
	tlimit= 4;				/* 4 attempts for connect */
	first_limit= 60;			/* default for 1st time callers, */
	_klimit= 1000;				/* 1M download limit, */
	_dlimit= 8 * 60;			/* 8 hr daily limit, */
	wlimit= 0;				/* not warned yet, */
	def_priv= NORMAL;			/* normal privelege, */
	cd_bit= 0x10;				/* CD bit on Async Card CTS */
	cd_flag= 0;				/* do not ignore CD */
	xferdisp= 1;				/* enable file xfer display */
	typemode= 0;				/* no type mode */
	mdmtype= HAYES;				/* default hayes modem */
	rate= 0;				/* default 300 baud */

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
					tlimit= atoi(*argv);
					cprintf("%u maximum attempts (with connect)\r\n",tlimit);
				} else cprintf("Bad attempt limit\r\n");

			} if (*p == 'X') {
				do_mail= 0;
				cprintf("No Outgoing Mail\r\n");

			} if (*p == 'T') {
				cprintf("Test mode\r\n");
				test= 1;

			} if (*p == 'M') {
				cprintf("Private message system\r\n");
				bbs= 0;

			} if (*p == 'L') {
				nlimit= atoi(*argv);
				if (nlimit == 0) cprintf("No");
				else cprintf("%u minute",nlimit);
				cprintf(" normal caller limit\r\n");

			} if (*p == 'D') {
				_dlimit= atoi(*argv);
				cprintf("%u minute daily time limit\r\n",_dlimit);

			} if (*p == 'K') {
				_klimit= atoi(*argv);
				cprintf("%uK byte daily download limit\r\n",_klimit);

			} if (*p == 'F') {
				first_limit= atoi(*argv);
				if (first_limit == 0) cprintf("No");
				else cprintf("%u minute",first_limit);
				cprintf(" first time caller limit\r\n");

			} if (*p == 'B') {
				ding= 0;
				cprintf("No bells on console.\r\n");

			} if (*p == 'P') {
				cprintf("Low first time caller privelege\r\n");
				def_priv= DISGRACE;

			} if (*p == 'S') {
				slimit= atoi(*argv);
				if (slimit < 5) slimit= 5;
				cprintf("%u minute signon time limit\r\n",slimit);

			} if (*p == 'U') {
				xferdisp= 0;
				cprintf("No File Transfer Debug Info\r\n");

			} if (*p == '?') {
				cprintf("DEBUG Enabled\r\n");
				debug= 1;

			} if (*p == 'R') {
				rate= 1;

			} if (*p == 'J') {
				switch(atoi(*argv)) {
					case 1:
						mdmtype= HAYES;
						cprintf("DC Hayes Modem\r\n");
						break;

					case 2:
						mdmtype= DF03;
						cprintf("DEC DF03 Modem\r\n");
						break;

					case 3:
						mdmtype= VA212;
						cprintf("Racal Vadic VA212 Modem\r\n");
						break;

					case 0:	/* just /J */
					case 4:
						mdmtype= SMARTCAT;
						cprintf("PC Junior/Novation Modem\r\n");
						break;
				}
			}
			++p;
		}
	}

/* Open the user file. Error if not there. */

	f= _xopen("user.bbs",0);
	if (f == -1) {
		cprintf("ERROR: No user list.\r\n");
		exit(2);
	}
	_xclose(f);

/* Count the number of active paths. */

	syscount();

/* Read all the command privelege level tables. No error if not there or
wrong length. (For backwards compatibility.) */

	v= _xopen("mainpriv.bbs",0);
	if (v != -1) {
		_xread(v,&main_priv,sizeof(main_priv));
		_xclose(v);
	}
	v= _xopen("msgpriv.bbs",0);
	if (v != -1) {
		_xread(v,&msg_priv,sizeof(msg_priv));
		_xclose(v);
	}
	v= _xopen("mailpriv.bbs",0);
	if (v != -1) {
		_xread(v,&mail_priv,sizeof(mail_priv));
		_xclose(v);
	}
	v= _xopen("filepriv.bbs",0);
	if (v != -1) {
		_xread(v,&file_priv,sizeof(file_priv));
		_xclose(v);
	}

/* Open the mail system file. If not there, create one. */

	v= _xopen("mail.sys",0);
	if (v == -1) {
		cprintf("- Creating MAIL.SYS\r\n");
		v= _xcreate("mail.sys",2);
		mail.fudge= 1.00;			/* default fudge factor, */
		mail.rate= 300;				/* default baud rate, */
		mail.node= -1;				/* new node, */
		*mail.mailpath= '\0';
		*mail.filepath= '\0';
		_xwrite(v,&mail,sizeof(mail));		/* write it out, */
	} else {
		f= _xread(v,&mail,sizeof(mail));
		if (f < sizeof(mail)) {			/* if pre v9 mail.sys */
			strcpy(mail.filepath,mail.mailpath); /* dup strings */
		}
	}
	_xclose(v);

/* Load the scheduler time table, or create a default one. */

	get_sched();

/* This is a loop, even though it doesnt look like one. Whenever a caller
is terminated, the frc_abort function causes control to be passed back 
to here. */

	_break(1,0);					/* disable BREAK */
	set_clk();					/* install clock */
	if (!test) {
		init();					/* start up hardware, */
		baud(rate);
		if (mdmtype != SMARTCAT)
			init_modem("fidomdm.bbs");	/* start up the modem */
	}
	doscode= 0;					/* return code */

	while (doscode == 0) {
		if (bdos(6,0xff) == 3) {		/* if control-C, */
			doscode= 1;			/* abort, */
			break;
		}

		event= in_sched('#');
		if (event != -1) {			/* if event to run, */
			tag= sched[event].tag;		/* do it now, */
			if ((sched[event].result >= MIN_EVENT)
			    && (sched[event].result <= MAX_EVENT)) {
				mode= FIDONET;
				do_net();		/* execute Fidonet, */

			} else if (sched[event].result > MAX_EVENT) {
				doscode= sched[event].result;
				break;
			}
		}

		cd_flag= 0;				/* real Carrier Detect */
		if (dsr()) {
			mode= FIDO;
			klimit= _klimit;
			dlimit= _dlimit;		/* real limits */
			do_fido();			/* if carrier do Fido */
		}
	}

	reset_clk();			/* turn off clock, */
	if (!test) uninit();		/* turn off hardware */
	lower_dtr();			/* disable modem */
	exit(doscode);			/* back to DOS */
}
