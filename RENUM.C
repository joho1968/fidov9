#include <stdio.h>
#include <ctype.h>
#include <bbs.h>

/* BBS message file renumberer. Speeds up the system by renumbering
the messages so they are all sequential. */

int highest;		/* highest message number currently in system, */
int count;		/* number of messages, */

int xl[1000];		/* assumes initially all zeros */

struct _msg mesg;

main() {

int msgnbr,newmsg,i,msgfile;
char name[14],newname[14];

	printf("Fido Message Renumberer (Packer) Version %s\r\n",VER);
	printf("All messages will be renumbered so they are all sequential.\r\n");
	printf("You must be logged into the message directory for this to work.\r\n");
	printf("Also handles replies, etc properly.\r\n");
	printf("Type RETURN to continue, or ^C to abort.\r\n");
	lconin();

	msgcount();	/* count em, */
	printf("%u messages in system. Renumbering takes three passes.\r\n",count);

	printf("Pass 1: Renaming all to working names,\r\n");
	for (msgnbr= 1; msgnbr <= highest; msgnbr++) {	/* rename all first, */
		sprintf(name,"%u.msg",msgnbr);	/* orig name, */
		sprintf(newname,"%u.num",msgnbr);
		_xrename(name,newname);		/* rename, ignore error, */
	}

	newmsg= 1;
	msgnbr= 1;
	printf("Pass 2: Renumbering, creating translate list.\r\n");
	i= count;
	while (i--) {
		msgnbr= findmsg(msgnbr,".num");	/* find a message, */
		sprintf(name,"%u.num",msgnbr);	/* make original name, */
		sprintf(newname,"%u.msg",newmsg);/* new name, */
		printf("Msg #%u becomes #%u\r\n",msgnbr,newmsg);
		_xrename(name,newname);
		xl[msgnbr]= newmsg;		/* note new number, */
		++msgnbr; ++newmsg;		/* next ... */
	}

	msgnbr= 1;
	printf("Pass 3: Translating Replies\r\n");
	i= count;
	while (i--) {
		msgnbr= findmsg(msgnbr,".msg");	/* find a message, */
		printf("Msg #%u: ",msgnbr);
		sprintf(name,"%u.msg",msgnbr);
		msgfile= _xopen(name,2);
		if (msgfile == -1) {
			printf("HORRIBLE ERROR: Cant open file. (Enough handles?)\r\n");
			printf("All replies disconnected. Fix the problem,\r\n");
			printf("run again, and they will all be cleared.\r\n");
			exit();
		}
		_xread(msgfile,&mesg,sizeof(mesg)); /* read header, */
		if (mesg.reply || mesg.up) {

			mesg.reply= xl[mesg.reply]; /* translate to new */
			mesg.up= xl[mesg.up];	/* msg numbers, */

			if (mesg.reply) printf("Is Reply to #%u ",mesg.reply);
			if (mesg.up) printf("Has Reply #%u",mesg.up);

			_xseek(msgfile,0L,0);	/* seek to start, */
			_xwrite(msgfile,&mesg,sizeof(mesg));
		}
		_xclose(msgfile);
		printf("\r\n");
		++msgnbr;
	}
	printf("Message renumbering complete.\r\n");
}
		
/* Count the number of messages and the highest message number, update the
system file array. */

msgcount() 
{
long fsize;
char name[14];
int n;

	count= 0;
	highest= 0;
	while (_xfind("*.msg",name,&fsize,0,count)) {
		++count;			/* count another, */
		name[strlen(name) - 4]= '\0';	/* delete the .msg part, */
		n= atoi(name);
		if (n > highest) highest= n;	/* pick highest one, */
	}
}

findmsg(num,ext)
int num;
char *ext;
{
char name[80];
int file;

	while (1) {
		if (num > highest) return(0);
		sprintf(name,"%u",num);
		strcat(name,ext);
		file= _xopen(name,0);
		if (file != -1) {
			_xclose(file);
			return(num);
		}
		++num;
	} 
	return(0);
}
