#define NUL 0
#define CNTRLB 2
#define CNTRLC 3
#define CNTRLE 5
#define TAB 9
#define LF 0x0a
#define CR 0x0d
#define CNTRLQ 17
#define CNTRLS 19
#define CNTRLZ 26
#define DEL 127
#define BS 8
#define ESC 0x1b

#define ABORT CNTRLC
#define FERROR -2

/* Characters used for file transmission. */

#define EOT 4			/* no more blocks or files */
#define ACK 6			/* good acknowledge */
#define SYN 22			/* start of info block */
#define SOH 1			/* start of data block */
#define NAK 21			/* bad acknowledge */
#define CRC 'C'			/* send in CRC mode, */
#define ETB 23			/* resend block */
#define TIMEOUT -1		/* no char received */
#define OKNMCH ACK		/* good filename acknowledge */
#define BDNMCH 'u'		/* bad filename acknowledge */
#define EOFCHAR CNTRLZ		/* end of filename */

/* Fancy numbers for TELINK internal stuff. */

#define CPMSIZ 128 		/* CP/M record size, */
#define TELINK 1		/* file transmission modes, */
#define MODEM7 2
#define XMODEM 3

/* Sort of logical things. */

#define	FALSE 0
#define	TRUE -1
#define	NONE 0
#define	ERROR -1
#define OK 0

char buffer[CPMSIZ];			/* file I/O buffer, */

/* File transmission flags, etc. */

unsigned crcflag;
unsigned crcmode;			/* 1 if CRC mode, */
unsigned filemode;			/* transfer type; XMODEM, MODEM7, TELINK */
char statflg;				/* 1= send file stats, 0= dont */
char batflg;				/* 1= batch mode, 0= single mode, */
unsigned chksum;			/* file transmission checksum */
unsigned sector;			/* sector being sent/recieved */
unsigned newsec;			/* sector # from the other modem */
unsigned secchk;			/* sector check from other modem */
unsigned newsum;			/* checksum from other modem */
unsigned firstchar;			/* first block char during file rcv */
unsigned ackchar;			/* ACK/NAK for current block, */
unsigned blknum;			/* displayed block number */
unsigned errors;			/* # block transmission	errors */
unsigned retrymax;			/* max. blk transmission errors */

/* Statistics on file transmission. */

unsigned totl_files;			/* total files sent/rec'd */
unsigned totl_failures;			/* how many failed, */
unsigned totl_errors;			/* error count, soft errors incl */
unsigned totl_blocks;			/* number blocks sent, */
