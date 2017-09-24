/* FidoNet Mail definitions. */

/* MAIL.SYS file structure */

struct _mail {
int node;		/* local node number, */
float fudge;		/* cost fudge factor, */
int rate;		/* baud rate */
char mailpath[80];	/* path to find mail in */
char filepath[80];	/* mail file path */
} mail;

/* Node descriptor. This is created from the routing list. */

struct _node {
int number;		/* node number, */
int cost;		/* cost per minute to call */
int rate;		/* baud rate */
char sched;		/* schedule tag */
char name[14];		/* node name */
char phone[40];		/* phone number */
char city[40];		/* city and state, */
} node;
