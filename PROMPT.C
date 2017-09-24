#include <stdio.h>
#include <ctype.h>
#include <bbs.h>

/* Change the privelege levels for the Main Commands, write it back out. */

set_main(p)
char *p;
{
int f;
	f= _xopen("mainpriv.bbs",2);		/* open the file, */
	if (f == -1) 				/* if not exist, make one, */
		f= _xcreate("mainpriv.bbs",2);
	cmd_table(p,&main_priv);		/* process commands, */
	_xwrite(f,&main_priv,sizeof(main_priv));
	_xclose(f);
}
/* Change the privelege levels for the msg Commands, write it back out. */

set_msg(p)
char *p;
{
int f;
	f= _xopen("msgpriv.bbs",2);		/* open the file, */
	if (f == -1) 				/* if not exist, make one, */
		f= _xcreate("msgpriv.bbs",2);
	cmd_table(p,&msg_priv);			/* process commands, */
	_xwrite(f,&msg_priv,sizeof(msg_priv));
	_xclose(f);
}
/* Change the privelege levels for the mail Commands, write it back out. */

set_m_msg(p)
char *p;
{
int f;
	f= _xopen("mailpriv.bbs",2);		/* open the file, */
	if (f == -1) 				/* if not exist, make one, */
		f= _xcreate("mailpriv.bbs",2);
	cmd_table(p,&mail_priv);		/* process commands, */
	_xwrite(f,&mail_priv,sizeof(mail_priv));
	_xclose(f);
}
/* Change the privelege levels for the file Commands, write it back out. */

set_file(p)
char *p;
{
int f;
	f= _xopen("filepriv.bbs",2);		/* open the file, */
	if (f == -1) 				/* if not exist, make one, */
		f= _xcreate("filepriv.bbs",2);
	cmd_table(p,&file_priv);		/* process commands, */
	_xwrite(f,&file_priv,sizeof(file_priv));
	_xclose(f);
}

/* Display and change the table of command names. */

cmd_table(p,t)
char *p;
struct _cmd *t;
{
char ps[80];

	if (num_args(p) < 2) {			/* if not enough args, */
		while (*t-> name) {
			mprintf("%-15s ",t-> name);
			getpriv(t-> priv);
			mprintf("\r\n");
			++t;
		}

	} else if (*p == '?') {
		mprintf("Change/display command privelege levels. To change,\r\n");
		mprintf("enter the command letter followed by the priv level:\r\n");
		mprintf("3 A NORMAL  or  3 R PRIVEL, etc. The levels are: TWIT,\r\n");
		mprintf("DISGRACE, NORMAL, PRIVEL, EXTRA, SYSOP.\r\n");

	} else {				/* else attempt to change */
		while (*t-> name) {
			if (tolower(*p) == tolower(*t-> name)) {
				cpyarg(ps,next_arg(p));
				stolower(ps);
				setpriv(ps,&t-> priv);
				break;
			}
			++t;
		}
	}
}
/* Build a menu from the structure passed. This accomodates the
set user screen width. If flag is set, then it lists only the first
char of each command name. Commands are only listed if the privelege
level is high enough. */

prompt(t,flag)
struct _cmd *t;
int flag;
{

int width;
int maxw;

	maxw= user-> width;
	if (maxw > 40) maxw= 40;

	width= 0;
	while (*t-> name) {
		if (user-> priv >= t-> priv) {
			if (flag) {
				mprintf("%c ",*t-> name);
			} else {
				mprintf("%s ",t-> name);
				width+= strlen(t-> name);
			}
		}
		++t;
		if ((width + strlen(t-> name)) >= maxw) {
			mprintf("\r\n");
			width= 0;
		}
	}
}

/* Return true if the character is a command in the table, and the
users privelege level is high enough. */

is_cmd(c,t)
char c;
struct _cmd *t;
{
	while (*t-> name) {
		if ((tolower(c) == tolower(*t-> name)) && (user-> priv >= t-> priv) )
			return(1);
		++t;
	}
	return(0);
}
