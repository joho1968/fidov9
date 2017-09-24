#include <stdio.h>
#include <ctype.h>
#include <bbs.h>
#include <mail.h>

/* Message area menu. */

msg_menu(s)
char *s;
{
char ln[80];
char path[80],cmd,*p;
int n;

#define MAIL (sys-> attrib & SYSMAIL)


	system(0,msgnum);		/* read system file, */
	if (!mail_checked[msgnum]) {
		mprintf("Msg Area #%u: %s %s\r\n",
			msgnum,sys-> msgpath,(MAIL ? "(FidoNet)" : ""));
		chk_mail(s);		/* area, check for mail, */
		mail_checked[msgnum]= 1; /* dont do every time. */
	}

	while (1) {
		abort= 0;
		line= 0;
		mprintf("\r\n");
		stoupper(sys-> msgpath);
		mprintf("Msg Area #%u: %s %s\r\n",
		msgnum,sys-> msgpath,(MAIL ? "(FidoNet)" : ""));
		if (user-> help > REGULAR) {
			if (MAIL) prompt(&mail_priv,0); 
			else prompt(&msg_priv,0);
			mprintf("\r\n");
		}
		if (user-> help > EXPERT) {
			if (MAIL) {
				mprintf("Mail: "); prompt(&mail_priv,1);
			} else {
				mprintf("Msg: "); prompt(&msg_priv,1);
			}
			mprintf("or ? for help: ");
		} else
			if (MAIL) mprintf("Mail Command: ");
			else mprintf("Msg Command: ");

		getstring(ln);
		mprintf("\r\n\r\n");
		p= skip_delim(ln);
		if (num_args(p) < 1) continue;

		stolower(p);
		cmd= *p++;
		p= skip_delim(p);
		p= strip_path(path,p);

/* We test for sysop-only commands for safety. If the privelege table files
are in an accessible area, it is possible for a knowledgeable idiot to
change them. They still wont get sysop commands. Note that this assumes
that all user commands are not numeric. */

		if (is_cmd(cmd,&msg_priv) || (cmd == '?') || (isdigit(cmd) && (user-> priv >= SYSOP))) {
			switch(cmd) {
				case 'a':
					if (isdigit(*p)) n= atoi(p);
					else (n= -1);
					if (msgsel(n)) {
						if (!mail_checked[msgnum]) {
							if (*p) p= skip_delim(++p);
							chk_mail(p);
							mail_checked[msgnum]= 1;
						}
					}
					break;
				case 'i':
					mfind(p);
					break;
				case 'k':
					mkill(p);
					break;
				case 'l':
					mlist(p);
					break;
				case 'r':
					mread(p);
					break;
				case 'e':
					msend("",0,0);
					break;
				case 'g':
					goodbye(p);
					break;
				case 'm':
					return;
					break;
				case 's':
					if (MAIL) {
						mprintf("----- FidoNet Stats -----\r\n");
						user_credit();
						mprintf("\r\n");
						mprintf("Available FidoNet Systems:\r\n");
						list_nodes();
						mprintf("------------------------\r\n");
					}
					mprintf("%u messages: 1 to %u\r\n",msg_total,msg_highest);
					stats();
					chk_mail(p);
					break;
				case '?':
				case 'h':
					if (MAIL) hlpmsg("mail");
					else hlpmsg("msg");
					break;
				case '1':
					lprintf("PATH path= %s p=%s\r\n",path,p);
					spath(p,path);
					mprintf("Wait ...\r\n");
					msgcount();
					break;
				case '2':
					mtest(p);
					break;
				case '3':
					lprintf("SETMSG =%s\r\n",p);
					if (MAIL) set_m_msg(p);
					else set_msg(p);
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
/* Ask to check messages in this area. */

chk_mail(s)
char *s;
{
char ln[82];

	while ((tolower(*s) != 'y') && (tolower(*s) != 'n')) {
		if (user-> help > EXPERT) mprintf("Want to check for mail? (y,n): ");
		else mprintf("Check mail (y,n): ");
		getstring(ln);
		mprintf("\r\n");
		s= skip_delim(ln);
	}
	if (tolower(*s) == 'y') listmsgs();
}
