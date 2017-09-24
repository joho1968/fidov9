/* Check for illegal filenames. Mostly just device names. */

char __devname[][8] = {
	"aux","prn","con","com1","com2",
	"lpt1","lpt2","clock","$clock","nul",
	""
};

char *strip_path();

bad_name(s)
char *s;
{
int i;
char name[80],*p;

	cpyarg(name,strip_path(name,s));/* check name only, */
	stolower(name);			/* all lower case, */
	p= name;
	while (*p) {			/* truncate the name at */		
		if ((*p == '.') || (*p == ':')) {
			*p= '\0';	/* colon or dot, */
			break;
		}
		++p;
	}

	for (i= 0; *__devname[i]; i++) {
		if (strcmp(name,__devname[i]) == 0) return(1);
	}
	return(0);
}
