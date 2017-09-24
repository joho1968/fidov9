#include <a:/drivers/driver.h>

/* These are in XENIX.LIB */

char *skip_delim();
char *next_arg();
char *strip_path();
long _fsize();
long _xseek();
long sizmem();
char *getml();

#define VER "9"			/* BBS Version number */

char copyright[] = "T. Jennings (c) 1983";

/* Callin users record. */

struct  _usr {
	char name[36];		/* users ASCII name, */
	char city[36];		/* city and state, */
	int date[20];		/* initial date, */
	char pwd[16];		/* what else, */
	int times;		/* # times called, */
	int help;		/* last help setting, */
	int tabs;		/* 1 == expand tabs, */
	int nulls;		/* number of nulls after CR, */
	int msg;		/* last selected message area, */
	int more;		/* last MORE setting, */
	int priv;		/* user privelege level, */
	char ldate[20];		/* last time called, */
	int time;		/* total time on system in 1 day, */
	unsigned flag;		/* various bit flags, */
	unsigned upld;		/* total K byte uploaded, */
	unsigned dnld;		/* total K bytes downloaded, */
	unsigned dnldl;		/* download, for limiting, */
	int files;		/* last selected file area, */
	char width;		/* screen width, */
	char len;		/* screen length, */
	int credit;		/* credit, in cents, */
	int debit;		/* debit, in cents, */
};

struct _usr *user;		/* pointer to user record */
struct _usr *userx;		/* ditto for U command */

#define TWIT -2			/* total asshole */
#define DISGRACE 0		/* disgraced user, */
#define NORMAL 2		/* normal user, */
#define PRIVEL 4		/* priveleged user, */
#define EXTRA 6			/* extra priveleges, */
#define SYSOP 10		/* SYSOP proveleges, */

#define EXPERT 2		/* user help level: expert */
#define REGULAR 4		/* experienced */
#define NOVICE 6		/* new user */

#define QUEST 1			/* flag: questionaire requested */

/* Structure for the time log: first access, last access, and an integer
counter for each hour of each day of the week. */

struct _tlog {
	int calls;		/* # calls in log, */
	char fdate[20];		/* logging started, */
	char ldate[20];		/* logging ended, */
	int log[7][24];		/* the counters, */
};

struct _tlog *tlog;

/* Structure for each virtual bulletin board. */

struct _sys {
	int caller;		/* caller number, */
	int priv;		/* min. privelege to access this */
	char msgpath[40];	/* path for message base, */
	char bbspath[40];	/* path for .BBS files, */
	char hlppath[40];	/* path for HLP files, */
	char uppath[40];	/* path for uploads, */
	char filepath[40];	/* path for file area, */
	int attrib;		/* attributes */
};
struct _sys *sys;
#define SYSMAIL 1		/* is a mail area */

/* Tables of command names and privelege levels for each. */


struct _cmd {
	char name[20];
	int priv;
};

struct _cmd main_priv[] = {
	"Msg-Section",TWIT,
	"File-Section",DISGRACE,
	"Goodbye",TWIT,
	"Statistics",TWIT,
	"Ans-Questionaire",TWIT,
	"Bulletin",TWIT,
	"Yell",DISGRACE,
	"Change",DISGRACE,
	"User-List",NORMAL,
	"",0
};	

struct _cmd msg_priv[] = {
	"Area-Change",NORMAL,
	"List",TWIT,
	"Read",TWIT,
	"Enter",TWIT,
	"Kill",NORMAL,
	"Index",TWIT,
	"Statistics",TWIT,
	"Goodbye",TWIT,
	"Main-Menu",TWIT,
	"",0
};
struct _cmd mail_priv[] = {
	"Area-Change",NORMAL,
	"List",TWIT,
	"Read",TWIT,
	"Enter",TWIT,
	"Kill",NORMAL,
	"Index",TWIT,
	"Statistics",TWIT,
	"Goodbye",TWIT,
	"Main-Menu",TWIT,
	"",0
};

struct _cmd file_priv[] = {
	"Area-Change",NORMAL,
	"Locate",NORMAL,
	"Files",NORMAL,
	"Type",NORMAL,
	"Goodbye",TWIT,
	"Upload",NORMAL,
	"Download",NORMAL,
	"Statistics",TWIT,
	"Main-Menu",TWIT,
	"Raw-Display",PRIVEL,
	"Kill-File",SYSOP,
	"",0
};

/* Static buffers for low level functions to avoid overflowing
the stack. */

char *_txbuf;			/* message display buff, ASCII upload */
#define _TXSIZE 32768		/* size for above */

char *text;			/* message text */
#define LINES 47		/* number of lines, */
#define COLS 82			/* number of columns */

/* Message header structure. The message text is just a long string. */

struct _msg {
	char from[36];		/* who from, */
	char to[36];		/* who to, */
	char subj[72];		/* message subject, */
	char date[20];		/* creation date, */
	int times;		/* number of times read, */
	int dest;		/* destination node, */
	int orig;		/* originating node */
	int cost;		/* actual cost of this msg */
	int caca[6];		/* extra space, */

	unsigned reply;		/* thread to previous msg. (reply-to) */
	int attr;		/* message tye, see below */
	int up;			/* thread to next msg. (replied-to) */
};
struct _msg *msg;		/* pointer to message header */

#define MSGPRIVATE 1		/* private message, */
#define MSGBROAD 2		/* broad cast message, */
#define MSGREAD 4		/* read by addressee */
#define MSGSENT 8		/* sent OK (remote) */
#define MSGFILE 16		/* file attached to msg */

/* Table of flags, whether "Check for mail?" has been asked yet. */

char mail_checked[99];		/* assumes cleared to 0 */

int sysfiles;			/* total system files, */
int pathnum;			/* current FILE path being used, */
int msgnum;			/* current MESSAGE path, */

unsigned msg_highest;		/* highest msg # in system, */
unsigned msg_total;		/* total number of messages, */

int userfile;			/* user file handle, */
int logfile;			/* console log file, */
int cmtfile;			/* logon and comment file, */
long userpos;			/* user file index, */

int mode;			/* fido/fidonet */
#define FIDO 1
#define FIDONET 2

int abort;			/* True if ^C typed. Stops mprintf() */
int doscode;			/* DOS error code */
int column;			/* column number, */
int line;
int do_mail;			/* 1 == send mail */
int echo;			/* 1 == echo on line input, */
int test;			/* 1 == test mode, (no modem) */
int bbs;			/* 1 == normal BBS, else private */
int ding;			/* 1 == bell to local console, */
int userflag;			/* 1 == OK to update user file, */
int logflag;			/* 1 == log file open, */
int cmtflag;			/* 1 == OK to update comment file, */
int typemode;			/* 1 == simultaneous keyboards */
int debug;			/* 1 == debug options */
int overwrite;			/* 1 == allow overwriting files */
int xferdisp;			/* 1 == display up/dn load status */

unsigned limit;			/* time limit in force, */
unsigned slimit;		/* signon time limit */
unsigned nlimit;		/* normal users limit, */
unsigned first_limit;		/* limit for 1st time callers, */
unsigned dlimit;		/* daily maximum time limit, */
unsigned klimit;		/* 48 hr K byte limit, */
unsigned tlimit;		/* attempt with connect limit */
int wlimit;			/* true if warned of time left */
int wover;			/* 1 == no final time limit message */

#define HAYES 0			/* DC Hayes and clones */
#define DF03 1			/* DEC DF 03 */
#define VA212 2			/* Racal Vadic VA 212 */
#define SMARTCAT 3		/* Novation SmartCat */
int mdmtype;			/* type of modem */

char date[20];			/* signon date, */

unsigned sysnum;		/* current system file, (1 command) */

int cd_flag;			/* true == ignore CD line */
int rate;			/* baud rate */

int def_priv;			/* default privelege for new users, */

int msgfile;			/* global handle for opened message, */
unsigned msgsize;		/* size of current message, */

char conbuf[64];		/* console type ahead buffer */
int head = 1;
int tail = 0;
