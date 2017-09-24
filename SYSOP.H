long _xseek();



/* Global stuff */

unsigned recd;			/* current record, */
int rdx;			/* index file handle, */
int hlp;			/* help file handle */
int changed;			/* true if record has been changed */

char linebuf[81];		/* local line editing, */

/* Screen variables, */

int lines;			/* number of lines, */
int cols;			/* number of columns, */
int bottom;			/* bottom screen line number, */

/* Character for drawing boxes. */

char ulcorner;			/* char for upper left corner */
char urcorner;			/* upper right corner, */
char llcorner;			/* lower left corner, */
char lrcorner;			/* lower right corner, */
char vertbar;			/* vertical bar, */
char horizbar;			/* horizontal bar, */
