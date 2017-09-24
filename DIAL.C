#include <stdio.h>
#include <ctype.h>
#include <bbs.h>
#include <a:/lib/ascii.h>

/*	.........................................
	.					.
	.	   Dialer For FidoNet		.
	.					.
	.	T. Jennings 26 July 84		.
	.	  created 18 July 84		.
	.					.
	.........................................

	Dialer module for FidoNet. Given a string, dial the
number, and return true if connected, else 0 if no connection.
The number passed is in telink/DC Hayes format.

	0,1,2,3,4,5,6,7,8,9,#,*
	( ) - ' '		ignored
	.			delay 1 sec
	P, T			Pulse, Touch Tone

result= dial(s);
char *s;		/* ptr to the phone # */

	This uses the global flag 'mdmtype' to determine how
to dial. Illegal values return 0, no connection.

	The function limitchk() checks the global time limits,
and if it times out aborts back to the caller, who of course
calls it 'no connection'. This is in case there is no modem, 
etc, to avoid hanging forever.

*/

dial(number)
char *number;
{
int result;

	switch(mdmtype) {

		case HAYES:
			result= hayes(number);
			break;

		case DF03:
			result= df03(number);
			break;

		case VA212:
			result= va212(number);
			break;

		case SMARTCAT:
			result= smartcat(number);
			break;

		default:
			cprintf("Unknown modem type == %u\r\n",mdmtype);
			result= 0;
			break;
	}
	return(result);
}
/* Dial for Hayes and clones. */

hayes(string)
char *string;
{
char c,*p;
int i;

	p= string;
	atp("ATDT",0);			/* dial command (default Touch Tone), */
	while (*p) {
		c= *p++;
		switch (c) {
			case 't':
				c= 'T';	/* make t and p T and P */
				break;
			case 'p':
				c= 'P';
				break;
			case '(':
			case ')':
			case ' ':
			case '-':	/* ignore these */
				c= '\0';
				break;
			case '.':
				c= ',';	/* dot means delay */
				break;
		}
		if (c != '\0') modout(c);
	}
	limit= 1;
	clr_clk();			/* one minute to dial */

	modout(CR);			/* start the command, */
	while (modin(10) != -1);	/* flush the modem, */

/* Now wait for a result code that says connect/no connect, and also
return true if we detect carrier. (I.e. no result codes set.) */

	do {				/* wait for result, */
		if (_dsr()) {		/* if CD, */
			delay(200);	/* delay for qubies, etc */
			return(1);	/* return connected */
		}
		limitchk();		/* check death timer */
		i= modin(20);		/* look for a numeric */
	} while (! isdigit(i));		/* result code */

	while (modin(10) != -1);	/* flush garbage, */
	return(result(i));		/* process digit */
}
/* Send a command sequence to the modem, */

atp(string,term)
char *string;
char term;
{
	while (*string) 		/* transmit string, */
		modout(*string++);
	if (term) 			/* if terminator, */
		modout(term);		/* send it, */
}
/* Return true if the character is a connect result. */

result(c)
char c;
{
	switch(c) {
		case '1':
			return(1);
		case '5':
			return(1);
		default:
			return(0);
	}
}
/* DEC DF03 Dialer. */

df03(string)
char *string;
{
char c,*p;
int i;

	baud(1);			/* dial at 1200 */
	p= string;
	modout(STX);			/* ^B dial command, */
	while (*p) {
		c= *p++;		/* process characters, */
		switch(c) {
			case '.':
				c= '=';	/* delay is = not . */
				break;
			case '(':
			case ')':	/* ignore these */
			case '-':
			case 'P':
			case 'p':
			case 'T':
			case ' ':
				c= '\0';
				break;
		}
		if (c != '\0') modout(c);
	}
	limit= 1;			/* 1 minute limit, */
	clr_clk();			/* start timing */
	flush();

/* Wait for the A or B result, return accordingly. The stupid thing
doesnt keep carrier detect properly, so if we get a connect, delay
for the CD line to settle. */

	while (1) {			/* wait for result, */
		limitchk();
		i= modin(20);
		if (i == 'A') break;
		if (i == 'B') return(0);
	}
	delay(300);			/* delay for CD settle */
	return(1);
}

/* Racal Vadic VA212 Dialer. */

va212(string)
char *string;
{
char c,*p;
int i;

	p= string;
	modout(ENQ); modout(CR);	/* ^E CR sequence, */
	flush();			/* wait for quiet, (end of prompt) */

	modout('D'); modout(CR);	/* send DIAL command, */
	flush();			/* wait for end of "NUMBER?" */

	p= string;			/* now send the phone number */
	while (*p) {
		c= *p++;		/* process characters, */
		switch(c) {
			case '.':	/* Fido delay char means */
				c= 'K';	/* wait for dial tone */
				break;
			case '(':
			case ')':	/* ignore these */
			case '-':
			case 'P':
			case 'p':
			case 'T':
			case 't':
			case ' ':
				c= '\0';
				break;
		}
		if (c != '\0') modout(c);
	}

	limit= 1;			/* 1 minute last resort timer */
	clr_clk();			/* clear clock, start timer */
	modout(CR);			/* start dialing with CR */
	flush();			/* flush the CR LF, any echo, etc */
	modout(CR);			/* acknowledge the number */

/* Its too difficult to check for the "ONLINE" message (ie. Im lazy)
so watch for either carrier detect, or a *, which means we got a 
prompt which means there is no connection. */

	while (1) {			/* wait for result, */
		if (_dsr()) break;
		limitchk();
		if (modin(20) == '*') {
			modout('I');	/* put into idle mode */
			modout(CR);
			return(0);
		}
	} 
	delay(200);			/* 2 sec for CD to settle */
	return(1);			/* OK, found CD true */
}
/* Flush the modem til its quiet for one second. */

flush() {

	while (modin(100) != -1) limitchk();
}
/* Novation SmartCat dialer. */

smartcat(string)
char *string;
{
char c,*p;
int i;

	p= string;
	atp("LONG 1",CR);		/* numeric results */
	flush();
	atp("DIAL ",0);			/* send DIAL, */

	p= string;
	while (*p) {
		c= *p++;		/* process characters, */
		switch(c) {
			case '.':
				c= 'W';	/* 5 sec wait */
				break;
			case '(':
			case ')':	/* ignore these */
			case '-':
			case ' ':
				c= '\0';
				break;
		}
		if (c != '\0') modout(c);
	}
	limit= 1;
	clr_clk();

	flush();			/* flush garbage, */
	modout(CR);			/* start with CR */

/* Wait for carrier detect, timeout, or a connect code. */

	while (1) {
		limitchk();
		i= modin(20);
		if (isdigit(i)) {
			if (i == '1') break;
			else return(0);
		} 
	}
	atp("UNLISTEN",CR);
	flush();
	return(1);
}
