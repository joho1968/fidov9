#include <stdio.h>
#include <ctype.h>
#include <bbs.h>

/* BBS Main Menu. Dispatch to system type commands, or the
message and file areas. */

main_menu() {

char ln[82];
char path[82];
char cmd,*p;

	while (1) {

		abort= 0;
		line= 0;
		mprintf("\r\n");
		if (user-> help > REGULAR) {
			mprintf("MAIN Commands:\r\n");
			prompt(&main_priv,0);
			mprintf("\r\n");
		}
		if (user-> help > EXPERT) {
			mprintf("Main: "); 
			prompt(&main_priv,1);
			mprintf("or ? for help: ");
		} else mprintf("Main Command: ");

		getstring(ln);				/* get a command, */
		mcrlf(); mcrlf();
		p= skip_delim(ln);			/* strip garbage, */
		if (num_args(p) < 1) continue;

		stolower(p);				/* make all lower case, */
		cmd= *p++;				/* get cmd char, */
		p= skip_delim(p);			/* skip any delims, etc */
		p= strip_path(path,p);			/* pull off path */

/* We test for sysop-only commands for safety. If the privelege table files
are in an accessible area, it is possible for a knowledgeable idiot to
change them. They still wont get sysop commands. Note that this assumes
that all user commands are not numeric. */

		if (is_cmd(cmd,&main_priv) || (cmd == '?') || (isdigit(cmd) && (user-> priv >= SYSOP))) {
			switch(cmd) {
				case 'm':
					msg_menu(p);
					break;
				case 'f':
					file_menu(p);
					break;
				case 'g':
					goodbye(p);
					break;
				case 's':
					stats();
					break;
				case 'a':
					if (question("question.bbs","answers.bbs") == 0)
						mprintf("Sorry, no questionaire\r\n");
					break;
				case 'b':
					bbsmsg("bulletin");
					break;
				case 'c':
					change(p);
					break;
				case 'y':
					chat();
					break;
				case 'u':
					users(p);
					break;
				case '?':
					hlpmsg("main");
					break;
				case '1':
					lprintf("PATH path= %s p=%s\r\n",path,p);
					spath(p,path);
					break;
				case '2':
					mtest(p);
					break;
				case '3':
					lprintf("SETMAIN =%s\r\n",p);
					set_main(p);
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
					mprintf("'%c' is not a command\r\n",cmd);
					break;
			}
		} else mprintf("'%c' is not a command\r\n",cmd);
	}
}
