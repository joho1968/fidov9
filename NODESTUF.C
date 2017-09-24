#include <stdio.h>
#include <ctype.h>
#include <bbs.h>
#include <mail.h>

/* Pull a particular node from the list. Return 0 if not found, and a cleared
structure. */

find_node(n)
int n;
{
int f;
char ln[132];

	if (n == 0) return(0);
	f= _xopen("nodelist.bbs",0);
	if (f == -1) return(0);

	make_node("");				/* clear in case empty file */
	while (_xline(f,ln,sizeof(ln) - 1)) {	/* read a line, */
		make_node(ln);			/* fill in the structure, */
		if (n == node.number) break;	/* if found, stop */
		make_node("");			/* invalidate node struct */
	}
	_xclose(f);
	return(node.number);
}

/* List the available nodes. return 0 if no node list. */

list_nodes()
{
int f;
char ln[132];

	f= _xopen("nodelist.bbs",0);
	if (f == -1) return(0);
	abort= 0;

	while (_xline(f,ln,sizeof(ln) - 1) && !abort) {
		if (*ln != ';') {
			make_node(ln);
			mprintf("%5u ... %-15s %-20s ",node.number,node.name,node.city);
			mprintf("Sched %c\r\n",node.sched);
		}
	}
	_xclose(f);
	return(1);
}
/* Given a text string, build the node descriptor structure. The string
is assumed to contain:

"NODENUM COST NAME PHONE CITY, ETC"

Where NODENUM is the node number, 1 to 65535, COST is the cost, in cents per
cubit per unit time, and NAME, PHONE and CITY are atoms within a text
string. These are delimited by our usual delimiters (set in BBSMAIN). */

make_node(s)
char *s;
{
char temp[80];

	node.number= 0;			/* clear in case bad string */
	node.cost= 0;
	*node.name= '\0';
	*node.phone= '\0';
	*node.city= '\0';
	node.rate= 0;
	node.sched= ' ';

	node.number= atoi(s);		/* get node number, */
	s= next_arg(s);

	node.cost= atoi(s);		/* cost per minute, */
	s= next_arg(s);

	node.rate= atoi(s);		/* baud rate, */
	s= next_arg(s);

	cpyatm(temp,s);			/* get temp. copy, */
	temp[sizeof(node.name) - 1]= '\0';/* force max. length, */
	strcpy(node.name,temp);
	s= next_arg(s);

	cpyatm(temp,s);			/* get copy of phone #, */
	temp[sizeof(node.phone) - 1]= '\0';
	strcpy(node.phone,temp);
	s= next_arg(s);

	cpyatm(temp,s);			/* "city" */
	temp[sizeof(node.city) - 1]= '\0';
	strcpy(node.city,temp);
	s= next_arg(s);

	if (*s == '\0')			/* copy schedule tag */
		node.sched= 'A';	/* default for old lists */
	else node.sched= toupper(*s);
}

/* Calculate the cost for N characters, at the current cost, baud rate
and fudge factor. Return as cost in cents. 

	message size, minutes =
		message size /			number of chars total,
		(baud rate / 10) * 60		chars per minute,

	message cost = 
		message size, minutes * cost per minute
 */

cost(n)
int n;
{
float msg_size,chars_min,cost;

	if (node.rate < 300) node.rate= 300;	/* error check div 0 */

	msg_size= n;				/* type conversion */

	chars_min= (node.rate / 10) * 60;	/* chars per minute, */

	cost= (msg_size / chars_min) * node.cost * mail.fudge;
	n= cost;				/* type conversion */
	if ((node.cost > 0) && (n < 1)) 	/* if toll call, $0.01 min */
		n= 1;
	return(n);
}
