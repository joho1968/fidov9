#include <stdio.h>
#include <ctype.h>
#include <bbs.h>

/* The questionaire module. 

Questions are just lines of text, where the first character is a marker that
indicates what to do with the line:

	/	Get a line of text from the user-> 
	+N	Multiple choice, 1 - N choices,
	?	Conditional question: if the last multiple choice
		answer was the last choice ("other") then
		get a line of text. Merely to allow A, B, C, other
		type questions.
	!	Conditional statement, operates the same as ?.
		If executed, it terminates the questionaire. Lets
		you ask if they want to fill it out or not.
	*	Put users name and signon date in answer file.
	_	Clear all conditional flags.

		Any other character indicates a prompt or
		other plain text.

* Enter users data,				recd header
Do you want to fill in a questionaire:		ask if continue,
+2 (1) Yes (2) No				get answer,
!						terminate if last ans.
/Describe your system. (One line)		text question
What comm program are you using:		plain text
+3(1)MINITEL (2)PC-TALK (3)Other:		multiple choice
?What is the name of your comm program? 	asked if answer above was 3
Thank you					plain text

 */

question(infile,outfile)
char *infile,*outfile;
{
int q;
int a;
char c;
char *ques,buf[80],ans[80];
int i,num,other;
int moresave;

	q= _xopen(infile,0);			/* open questionaire file, */
	if (q == -1) return(0);			/* skip it if not exist, */
	a= _xopen(outfile,2);			/* open answer file, */
	if (a == -1) {				/* if not there, */
		a= _xcreate(outfile,2);		/* make it, */
		if (a == -1) {			/* if cant, then */
			_xclose(q);		/* close up and return, */
			return(0);
		}
	} else _xseek(a,0L,2);			/* seek to EOF on open file, */

	moresave= user-> more; user-> more= 0;	/* suspend More? thing, */

	num= 0;
	while (_xline(q,buf,sizeof(buf))) {	/* process each question, */
		ques= buf;			/* 1st char of line, */
		if (*ques == '*') {		/* * means put in user info */
			sprintf(buf,"* %s %s\r\n",user-> name,user-> ldate);
			_xwrite(a,buf,strlen(buf));

		} else if (*ques == '+') {	/* + means multiple choice, */
			++num;			/* bump question number, */
			++ques;			/* skip marker character, */
			i= atoi(ques);		/* maximum choice, */
			if (i == 0) i= 1;	/* (if bad questionaire) */
			++ques;			/* skip number, */
			do {			/* get a number 1 to N */
				getfield(ans,ques,1,1,sizeof(ans),1);
			} while ((atoi(ans) < 1) || (atoi(ans) > i));
			other= 0;		/* for conditional questions */
			if (atoi(ans) == i) other= 1;
			sprintf(buf,"%4u: %s\r\n",num,ans);
			_xwrite(a,buf,strlen(buf));

		} else if (*ques == '/') {	/* text answer, */
			++num;
			++ques;			/* skip marker, */
			getfield(ans,ques,0,99,sizeof(ans),1);
			sprintf(buf,"%4u: %s\r\n",num,ans);
			_xwrite(a,buf,strlen(buf));

		} else if (*ques == '!') {
			if (other) break;	/* terminate if other */

		} else if (*ques == '?') {	/* conditional question */
			if (other) {		/* only if "other" selected */
				++ques;
				getfield(ans,ques,0,99,sizeof(ans),1);
				sprintf(buf,"?%3u: %s\r\n",num,ans);
				_xwrite(a,buf,strlen(buf));
			}
			other= 0;

		} else if (*ques == '_') {	/* clear flags, */
			other= 0;

		} else {			/* else just text, */
			mprintf("%s\r\n",ques);
		}
	}
	user-> more= moresave;
	line= 0;

	_xclose(q);
	_xclose(a);
	return(1);
}
