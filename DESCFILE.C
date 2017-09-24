#include <stdio.h>
#include <ctype.h>
#include <bbs.h>
#include <a:/lib/xfbuf.h>

/* Given a list of names, prompt for a description, and add it to FILES.BBS
in the current directory. */

descfile(path)
char *path;
{
char name[80],*p;
struct _xfbuf xfbuf;
int i,f;

	strcpy(name,sys-> uppath);
	strcat(name,"files.bbs");
	f= _xopen(name,2);		/* open files.bbs, */
	if (f == -1) f= _xcreate(name,2);/* make one if no exist */
	*name= ' ';			/* not ^Z, find end of file, */
	while (*name != 0x1a) {
		if (_xread(f,name,1) == 0) break;
	}
	if (*name == 0x1a) _xseek(f,-1L,1); /* seek back over any ^Z, */

	p= path;
	while (num_args(p)) {		/* for each item in the list, */
		cpyarg(name,p);		/* one at a time, */
		if (!wild(name)) {
			xfbuf.s_attrib= 0;
			i= 0;
			while (_find(name,i,&xfbuf)) {
				++i;
				dodesc(xfbuf.name,f);	/* describe each file, */
			}
		}
		p= next_arg(p);
	}
	_xclose(f);
}
/* Describe a file, add it to the FILES.BBS file. */

dodesc(s,f)
char *s;
int f;
{
char ln[80],name[14];

	cpyarg(name,s);
	stoupper(name);
	mconflush();			/* flush garbage from upload, */
	mprintf("Please describe %-13s: ",name);
	getstring(ln);
	mprintf("\r\n");
	strcat(name," ");		/* add a blank to the name, */
	_xwrite(f,name,strlen(name));	/* write name, */
	strcat(ln,"\r\n");
	_xwrite(f,ln,strlen(ln));	/* write description */
}
