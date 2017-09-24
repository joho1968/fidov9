#include <stdio.h>
#include <ctype.h>
#include <bbs.h>
#include <mail.h>
#include <pkthdr.h>
#include <nodemap.h>
#include <sched.h>

/* Assemble the packets of messages to be sent to the remote nodes. Return the
number of nodes created, and the array of nodenumbers all set. 

NOTE: This assumes that set_abort() was used somewhere, as thats how we
exit if the disk is full. */

make_packet()
{
int i,n,t,file,fll;
char filename[80],ln[132];

/* Fill the node map with all possible nodes. This includes only ones
with the right schedule tag. */

	file= _xopen("nodelist.bbs",0);
	if (file == -1) {
		cprintf("Missing the nodelist!\r\n");
		return(0);
	}
	nodetotal= 0;					/* none yet, */
	while (_xline(file,ln,sizeof(ln) - 1)) {	/* read a node line, */
		if (*ln != ';') {
			make_node(ln);			/* add nodes, */
			if ((node.number != mail.node)	/* except our own, */
			  && (node.sched == tag)) {	/* and right schedule */
				nmap[nodetotal++].number= node.number;
			}
		}
	}
	_xclose(file);
	cprintf("%u accessable FidoNodes\r\n",nodetotal);

/* Now go through the messages in the system, and mark the destination node
in the node map to indicate that there is mail to it. (I.e. need to make a 
packet later.) Broadcast messages go to all nodes. */

	cprintf("Making a list of nodes and messages\r\n");
	i= 1;
	t= 0;
	while(i= findmsg(i,1)) {
		if (((msg-> attr & MSGSENT) == 0) || bbs) {/* if not sent yet, */

			if ((msg-> attr & MSGBROAD) && (msg-> orig == mail.node)) {
				for (n= 0; n < nodetotal; n++) {
					++nmap[n].msgs;	/* mark everyone */
					++t;		/* count as a msg */
				}

			} else if ((msg-> dest != 0) && (msg-> dest != mail.node)
			   && (msg-> orig == mail.node)) {
				t++;
				for (n= 0; n < nodetotal; n++) {
					if (msg-> dest == nmap[n].number) break;
				}
				if (msg-> dest == nmap[n].number)
					++nmap[n].msgs;
			}
		}
		_xclose(msgfile);
		++i;
	}
	cprintf("%u outgoing messages\r\n",t);

/* Now go through the node map, once for each node in it. If there are
messages to be sent, create the packet. Have to do one at a time, as we'll 
probably run out of handles otherwise. Put any files (names) into the file
list for later processing. */

	for (i= 0; i < nodetotal; i++) {
		if (nmap[i].msgs > 0) {
			sprintf(filename,"%u.out",nmap[i].number);
			file= _xcreate(filename,2);
			if (file == -1) {
				cprintf("Cant create mail packet %s\r\n",filename);
				nodetotal= 0;
				return;
			}
			sprintf(filename,"%u.fll",nmap[i].number);
			fll= _xcreate(filename,1);
			if (fll == -1) {
				cprintf("Cant create file list %s\r\n",filename);
				nodetotal= 0;
				return;
			}
			make_header(file,nmap[i].number);
			n= 1;
			nmap[i].msgs= 0;		/* clear, set real count */
			while (n= findmsg(n,1)) {
				if ( (((msg-> attr & MSGSENT) == 0) || bbs)
				   && (msg-> dest != 0) 
				   && (msg-> dest != mail.node)
				   && ((msg-> dest == nmap[i].number) || (msg-> attr & MSGBROAD) )
				   && (msg-> orig == mail.node) ) {
					add_msg(file);
					++nmap[i].msgs;

					if (msg-> attr & MSGFILE) {
						_xwrite(fll,msg-> subj,strlen(msg-> subj));
						_xwrite(fll," ",1);
						++nmap[i].files;
					}
				}
				_xclose(msgfile);
				++n;
			}
			end_hdr(file);
			_xclose(file);
			_xclose(fll);
			cprintf("Fido #%u, %u messages, %u files\r\n",
				nmap[i].number,nmap[i].msgs,nmap[i].files);
		}
	}
	return(nodetotal);
}
/* Create the packet header, and write it to disk. Abort if the disk is full. */

make_header(file,n)
int file,n;
{
struct _hdr hdr;
int i;
	hdr.orig= mail.node;		/* assemble it all, */
	hdr.dest= n;			/* destination node */
	hdr.year= gtod3(0);		/* set date, etc */
	hdr.month= gtod3(1);
	hdr.day= gtod3(2);
	hdr.hour= gtod3(4);
	hdr.minute= gtod3(5);
	hdr.second= gtod3(6);

	for (i= 0; i < 20; i++) hdr.extra[i]= 0;
	if (_xwrite(file,&hdr,sizeof(hdr)) != sizeof(hdr))
		frc_abort();
}
/* Add the current message to the packet. Abort if disk full. */

add_msg(file)
int file;
{
int n;
char *p,buf[80],path[80];

	cputi(file,1);			/* type= message, */
	cputi(file,msg-> attr);		/* write attribute, */
	cputi(file,msg-> cost);		/* message cost, */
	cputs(file,msg-> date);		/* install date string, */
	cputs(file,msg-> to);
	cputs(file,msg-> from);
	if (msg-> attr & MSGFILE) {	/* if file names, */
		p= msg-> subj;
		*buf= '\0';		/* strip off all the path names */
		while (num_args(p)) {	/* put in only the file names */
			strcat(buf,strip_path(path,p));
			p= next_arg(p);
		}
		stoupper(buf);		/* make upper case */
		cputs(file,buf);

	} else cputs(file,msg-> subj);

	n= _xread(msgfile,text,_TXSIZE - 1);
	text[n + 1]= '\0';
	cputs(file,text);
}
/* Mark the end of the packet. */

end_hdr(file)
int file;
{
	cputi(file,0);
}
/* Put a string into the packet file.Note that this also writes the
null terminator. */

cputs(file,s)
int file;
char *s;
{
	if (_xwrite(file,s,strlen(s) + 1) != (strlen(s) + 1))
		frc_abort();
}
/* Write an integer to the packet. */

cputi(file,i)
int file,i;
{
	if (_xwrite(file,&i,sizeof(i)) != sizeof(i))
		frc_abort();
}
