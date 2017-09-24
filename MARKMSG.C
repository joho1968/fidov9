#include <stdio.h>
#include <ctype.h>
#include <bbs.h>
#include <mail.h>

/* After successful transmission of a packet to a given node, mark
all messages to that node as SENT. */

markmsg(noden)
int noden;
{
int i;

	i= 1;
	while (i= findmsg(i,1)) {
		if (msg-> dest == noden) {
			msg-> attr |= MSGSENT;
			wrtmsg();

		} else _xclose(msgfile);
		++i;
	}
}

/* Mark all broadcasts originating at this node as SENT. */

markbroad() {

int i;

	i= 1;
	while (i= findmsg(i,1)) {
		if ((msg-> orig == mail.node) && (msg-> attr & MSGBROAD)) {
			msg-> attr |= MSGSENT;
			wrtmsg();
		} else _xclose(msgfile);
		++i;
	}
}
