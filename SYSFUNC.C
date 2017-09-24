#include <stdio.h>
#include <ctype.h>
#include <bbs.h>

/* Fielded input. Inputs are:

string		where to put the input,
prompt		the prompt string,
min		minimum number of words,
max		maximum number of words,
width		maximum string size,
cap		Remove extra spaces, capitalize, etc. Works
		only if one or two words.
*/

getfield(string,prompt,gmin,gmax,width,cap)
char *string,*prompt;
int gmin,gmax,width,cap;
{
char buff[82];
int i;
char *p;
	while (1) {
		place(0,0);
		cprintf("%s",prompt);
		clreol();
		getstring(buff);
		if (strlen(buff) > width) {
			stat("Too long");
			continue;
		}

		if ((num_args(buff) > gmax) || (num_args(buff) < gmin)) {
			if (gmin != gmax) {
				sprintf(buff,"%d to %d words.\r\n",gmin,gmax);
				stat(buff);
			} else  {
				sprintf(buff,"%u word(s)\r\n",gmin);
				stat(buff);
			}
			continue;
		}
		if (cap) {
			p= skip_delim(buff);
			stolower(p);			/* make all lower case, */
			cpyarg(string,p);		/* copy in 1st name, */
			string[0]= toupper(string[0]);	/* capitalize, */
			p= next_arg(p);			/* skip that one, */
			while (num_args(p)) {
				strcat(string," ");	/* separate with a space, */
				*p= toupper(*p);
				cpyarg(&(string[strlen(string)]),p); /* add 2nd name, */
				p= next_arg(p);		/* ptr to next name, */
			}
		} else
			strcpy(string,buff);
		break;
	}
}
/* Display an integer as dollars and cents. The value passed is assumed to
be integer cents. */

dollar(n)
int n;
{
	cprintf("$%u.",n / 100);		/* whole dollars, */
	if ((n % 100) < 10) lconout('0');	/* make cents two digits */
	cprintf("%u",n % 100);
	if (n < 10000) lconout(' ');		/* make consistent width */
	if (n < 1000) lconout(' ');
}
/* Return true if s1 is contained within s2. This tends to match dissimilar
items, but is fast. The 0x5f mask is a crude toupper(). */

substr(s1,s2)
char *s1,*s2;
{
char *p,*q;

	while (*s2) {
		p= s1; q= s2;
		while ((((*p & 0x5f) == (*q & 0x5f)) || (*p == '?')) && *p) {
			++p; ++q;
		}
		if (*p == '\0') return(1);
		++s2;
	}
	return(0);
}
/* Draw a box, given the four corners. It is assumes that it is a
rectangle that we're trying to draw. */

box(topline,botline)
int topline,botline;
{
int i;
int topcol,botcol;

	topcol= 0;
	botcol= cols - 1;

	place(topline,topcol);
	for (i= botcol - topcol; i--;) lconout(horizbar);
	place(botline,topcol);
	for (i= botcol - topcol; i--;) lconout(horizbar);

	for (i= topline; i <= botline; i++) {
		place(i,topcol);
		lconout(vertbar);
		place(i,botcol);
		lconout(vertbar);
	}
	place(topline,topcol); lconout(ulcorner);
	place(topline,botcol); lconout(urcorner);
	place(botline,botcol); lconout(lrcorner);
	place(botline,topcol); lconout(llcorner);
}
