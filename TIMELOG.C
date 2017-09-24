#include <stdio.h>
#include <ctype.h>
#include <bbs.h>

/* display the time log array. */

char dayz[7][4] = { "Sun","Mon","Tue","Wed","Thu","Fri","Sat" };

int pozflg,numflg,prtflg;

main(argc,argv)
int argc;
char **argv;
{

int day,hour;
int f;
int m;
char sw[40],file[20];
char *p;
struct _tlog tlg;

	printf("Fido's Timelog Utility -- Version %s    (/? for help)\r\n",VER);
	pozflg= numflg= 0;
	strcpy(file,"");
	if (argc > 1) {
		strip_switch(sw,argv[1]);
		cpyarg(file,argv[1]);
		p= sw;
		while (*p) {
			switch (*p) {
				case 'P':
					pozflg= 1;
					break;
				case 'N':
					numflg= 1;
					break;
				case 'F':
					prtflg= 1;
					break;
				case '?':
					printf("/P Pause every graph\r\n");
					printf("/N Numeric Listing\r\n");
					printf("/F Format for a printer\r\n");
					printf("Optionally enter a filename \r\n");
					printf("to be used instead of TIMELOG.BBS\r\n");
					printf("i.e. TIMELOG 04APR84.BBS/P\r\n");
					break;
			}
			++p;
		}
	}

	if (strlen(file) == 0) strcpy(file,"timelog.bbs");
	f= _xopen(file,0);
	if (f == -1) {
		printf("Cant find %s\r\n",file);
		exit();
	}

	_xread(f,&tlg,sizeof(struct _tlog));
	_xclose(f);

	printf("%u calls, from %s  to  %s\r\n",tlg.calls,tlg.fdate,tlg.ldate);

	if (numflg) {
		printf("           Sun  Mon  Tue  Wed  Thu  Fri  Sat\r\n");
		for (hour= 0; hour < 24; hour++) {
			printf("%2u:00    ",hour);
			for (day= 0; day < 7; day++) {
				if (tlg.log[day][hour]) 
					printf("%5u",tlg.log[day][hour]);
				else printf("%5s","");
			}
			printf("\r\n");
		}
		pause();
	}

/* Now draw a crude graph. */

	printf("\r\n  Call Frequency by Day of the Week\r\n");
	printf("    0         10        20        30        40        60        70\r\n");
	printf("    |         |         |         |         |         |         |\r\n");
	for (day= 0; day < 7; day++) {
		m= 0;
		for (hour= 0; hour < 24; hour++) {
			m+= tlg.log[day][hour];
		}
		printf("%s |",dayz[day]);
		while (m--) printf("*");
		printf("\r\n");
	}
	printf("    |         |         |         |         |         |         |\r\n");
	pause();

	printf("\r\nCall Frequency by the Hour, each Day\r\n");
	for (day= 0; day < 7; day++) {
		printf("%s\r\n",dayz[day]);
		printf("      0         5         10        15        20        25        30\r\n");
		printf("      |         |         |         |         |         |         |\r\n");
		for (hour= 0; hour < 24; hour++) {
			printf("%2u:00 |",hour);
			m= tlg.log[day][hour];
			while (m--) printf(" *");
			printf("\r\n");
		}
		printf("      |         |         |         |         |         |         |\r\n");
		printf("\r\n");
		if (prtflg && (day % 1 == 0)) printf("\014");
		pause();
	}
	printf("\r\n\r\n");

}
/* If the pause flag is set, do th strike any key business. */

pause() {

	if (pozflg && ! prtflg) {
		cprintf("Strike any key . . . ");
		getch();
		cprintf("\r\n");
	}
}
