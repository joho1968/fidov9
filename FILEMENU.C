#include <stdio.h>
#include <ctype.h>
#include <bbs.h>
#include <xtelink.h>

/*	File system for BBS. */

/* Files subsystem. If dumb user, display help. Get commands, execute .*/

file_menu(s)
char *s;
{
char buff[80],*p;
char cmd;
char path[40];
int i,n;

	system(0,pathnum);			/* select right area, */

	while (1) {

/* If the upload path is the same  as the download path, dont
allow overwriting existing files. That would make a mess of the
files.bbs files list. */

		stoupper(sys-> filepath);	/* both to upper case, */
		stoupper(sys-> uppath);		/* if paths the same, */
		if (strcmp(sys-> filepath, sys-> uppath) == 0) 
			overwrite= 0;		/* no overwriting */
		else overwrite= 1;

		mprintf("File Area #%u: %s\r\n",pathnum,sys-> filepath);
		if (user-> help > REGULAR) {
			prompt(&file_priv,0);
			mprintf("\r\n");
		}
		if (user-> help > EXPERT) {
			mprintf("File: ");
			prompt(&file_priv,1);
			mprintf("or ? for help: ");
		} else mprintf("File Command: ");

		getstring(buff);		/* get command, */
		mprintf("\r\n\r\n");
		p= skip_delim(buff);		/* skip spaces, etc */
		if (num_args(p) < 1) continue;

		stolower(p);			/* lower case for compares, */
		cmd= *p++;
		p= skip_delim(p);		/* spaces, etc */

/* We test for sysop-only commands for safety. If the privelege table files
are in an accessible area, it is possible for a knowledgeable idiot to
change them. They still wont get sysop commands. Note that this assumes
that all user commands are not numeric. */

		if (is_cmd(cmd,&file_priv) || (cmd == '?') || (isdigit(cmd) && (user-> priv >= SYSOP))) {
			switch(cmd) {
				case 'm':
					return;
					break;
				case 'l':
					ffind(p);
					break;
				case 'a':
					if (isdigit(*p)) n= atoi(p);
					else n= -1;
					chdir(n);
					break;
				case 'k':
					delete(p);
					break;
				case 'r':
					dir(p);
					break;
				case 'f':
					file_disp(p,1);
					break;
				case 't':
					ftype(p);
					break;
				case 'u':		/* count uploaded K bytes, */
					xfer(0,p);	/* give credit on dnld limit */
					i= ((totl_blocks * 128) + 1023) / 1024;
					user-> upld+= i;
					if (user-> dnldl > 0) {
						if (i > user-> dnldl) user-> dnldl= 0;
						else user-> dnldl-= i;
					}
					break;
				case 'd':		/* count downloaded K bytes, */
					if ((user-> dnldl > klimit) && (klimit > 0)) {
						bbsmsg("dnldl");
						break;
					}
					xfer(1,p);
					user-> dnld+= ((totl_blocks * 128) + 1023) / 1024;
					user-> dnldl+= ((totl_blocks * 128) + 1023) / 1024;
					break;
				case '?':
					hlpmsg("files");
					break;
				case 's':
					freespace();
					stats();
					break;
				case 'g':
					goodbye(p);
					break;
				case '1':
					lprintf("PATH path= %s p=%s\r\n",path,p);
					spath(p,path);
					break;
				case '2':
					mtest(p);
					break;
				case '3':
					lprintf("SETFILE =%s\r\n",p);
					set_file(p);
					break;
				case '4':
					lprintf("SETMAIL path=%s p=%s\r\n",path,p);
					setmail(p,path);
					break;
				case '5':
					list_sched();
					break;
				case '6':
					new_sched();
					break;
				default:
					mprintf("'%s' is not a command.\r\n",cmd);
					break;
			}
		} else mprintf("'%c' is not a command\r\n",cmd);
	}
}
