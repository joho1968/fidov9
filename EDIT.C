#include <stdio.h>
#include <ctype.h>
#include <bbs.h>


char *skip_space();

/* Enter a block of text. Does word wrap, etc. Returns the highest line
written in. */

edit(sline,mline,width)
int width;
{
int ln,col,oldline;
char word[160],*p,c;		/* word[] must be at least 2X max line len */
int pc;

	ln= sline;		/* starting line, */
	col= 0;
	oldline= -1;
	text[ln * COLS]= '\0';	/* init our line */
	width-= 3;		/* our prompt length, */

	while (1) {

		if (ln != oldline) {
			mprintf("\r\n%2u: %s",ln + 1,&text[ln * COLS]);
			oldline= ln;
			line= 0;			/* override "More?" */
		}

		pc= 0;
		word[pc]= '\0';
		while (strlen(word) + strlen(&text[ln * COLS]) < width) {
			c= mconin() & 0x7f;		/* get one word */
			if (c == '\r') break;		/* stop if CR, */
			if (c == '\t') c= ' ';		/* make tabs usable */

			if ((c == '\010') || (c == '\177')) {
				if (pc > 0) {		/* backspace first */
					erase(1);	/* deletes chars in */
					mconout('\010'); /* our word, */
					word[--pc]= '\0'; 

				} else if (col > 0) {	/* or if empty, */
					erase(1);	/* then in the line */
					mconout('\010');
					text[(ln * COLS) + --col]= '\0';
				}

			} else if (c >= ' ') {		/* if printable, */
				mconout(c);
				word[pc++]= c;		/* else store until */
				word[pc]= '\0';		/* end of word or */
			}
			if ((c == ' ') || (c == ',')) break; /* too long */
		}

		if (strlen(word) + strlen(&text[ln * COLS]) < width) {
			strcat(&text[ln * COLS],word);	/* if it fits, */

		} else {				/* doesnt fit */
			strcat(&text[ln * COLS],"\215\n"); /* soft CR LF, */
			erase(strlen(word));		/* clear last word, */
			++ln;				/* next line, */
			strcpy(&text[ln * COLS],word); /* word on next line */
		}
		if (c == '\r') {			/* if hard return */
			strcat(&text[ln * COLS],"\r\n"); /* add CR LF */
			++ln;				/* if CR start */
			text[ln * COLS]= '\0';		/* a new line */
			if (strlen(&text[(ln - 1) * COLS]) == 2)
				break;			/* stop if empty line */
		}
		col= strlen(&text[ln * COLS]);		/* correct column */
		if (ln >= mline) break;
	}
	return(oldline);				/* last line edited */
}
/* Substitute one string for another. If the line becomes too long, it
gets truncated to fit. However, the line length is the physical array length, 
80 chars, so when listed, it will adjust properly if less than that. Returns
true if the substitution was made. */

substr(s,o,n,l)
char *s,*o,*n;
int l;
{
char work[200];
int m,i;

	m= strfnd(s,o);			/* find in string, old substr */
	if (m == -1) return(0);		/* not found */
	for (i= 0; i < m; i++) 		/* copy up to old, */
		work[i]= s[i];
	strcpy(&work[m],n);		/* put in new string, */
	m+= strlen(o);			/* skip past old string */
	strcat(work,&s[m]);		/* copy rest of string, */
	work[l]= '\0';			/* force max length, */
	strcpy(s,work);			/* update original string */
	return(1);
}
/* Find a substring within a string. Return its index, or -1 if not
found. Ignore case. */

strfnd(string,pattern)
char *string,*pattern;
{
int i;
char *s,*p;

	for (i= 0; i < strlen(string); i++) {		/* unanchored search, */
		s= &string[i]; 				/* start at each char */
		p= pattern;				/* position, */
		while (*s && *p && (tolower(*s) == tolower(*p))) { /* search until */
			++s; ++p;			/* mismatch or */
		}					/* end of pattern, */
		if (*p == '\0') return(i);		/* if end, found it */
	}
	return(-1);
}

/* Erase a word. Backspace, then type spaces. */

erase(n)
int n;
{
int i;

	for (i= 0; i < n; i++) mconout('\010');
	for (i= 0; i < n; i++) mconout(' ');
}
/* Skip leading blanks. */

char *skip_space(s)
char *s;
{
	while (*s && (*s == ' ')) ++s;
	return(s);
}
