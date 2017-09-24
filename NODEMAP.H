/* Table of nodes to call, and stats on them. */

#define MAXNODES 50

struct {
	int number;		/* the node number, 0 if blank */
	int tries;		/* number of attempts to send, */
	int connects;		/* number of actual connections */
	int time;		/* total time to send, */
	int success;		/* true if sucessful */
	int msgs;		/* number of messages in the packet, */
	int files;		/* files in the packet, */
} nmap[MAXNODES];

int nn;				/* current node, */
int nodetotal;			/* number of nodes in the table */
