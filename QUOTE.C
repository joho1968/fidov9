#include <stdio.h>
#include <ctype.h>

long _fsize();

/* Given a filename, randomly pick out one parargraph and type it. Paragraphs 
are assumed to be delimited by a CR LF CR LF sequence. Return 0 if no quote 
found.  */

quote(file)
char *file;
{
long size,pos;
int handle,i;
char time[20];
char mark[5];
char c;
int start,stop;

	size= _fsize(file);		/* get file size, */
	handle= _xopen(file,0);		/* open it, */
	if (handle == -1) return(0);	/* doesnt exist, */

	gtod(time);			/* random number, */
	pos= atoi(&time[11]) + (60L * atoi(&time[14]));
	pos*= atoi(&time[17]);
	pos*= 32L;			/* GUESS: minimum quote size, */
	pos%= size;			/* bound it, */
	_xseek(handle,pos,0);		/* random seek, */
	strcpy(mark,"XXXX");		/* anything but CR LF CR LF */
	start= 0;			/* not start of quote yet, */
	stop= 0;

	while (_xread(handle,&c,1) && !stop) {
		for (i= 0; i < 4; i++)	/* shift over, */
			mark[i]= mark[i + 1];
		mark[3]= c;		/* new character, */
		if (strcmp(mark,"\r\n\r\n") == 0) {
			if (start) stop= 1;	/* if not 1st CR LF CR LF, */
			else {			/* end of quote, */
				mprintf("\r\n\r\nQuote for the day:\r\n");
				start= 1;
			}
		}
		if (start) mconout(c);
	}
	mprintf("\r\n");
	_xclose(handle);
	return(start);
}
