#include <stdio.h>
#include <ctype.h>
#include <bbs.h>
#include <mail.h>

/* Settle all the debts in the user file. If there is an outstanding
debit, adjust the credit. */

markusr() {

int f;

	f= _xopen("user.bbs",2);
	if (f == -1) {
		cprintf("Cant find the user file!\r\n");
		return;
	}

	while (_xread(f,user,sizeof(struct _usr))) {
		if (user-> debit) {
			user-> credit-= user-> debit;
			user-> debit= 0;
			cprintf("%s: ",user-> name);
			dollar(user-> credit);
			cprintf(" credit\r\n");
			_xseek(f,- (long) sizeof(struct _usr),1);
			_xwrite(f,user,sizeof(struct _usr));
		}
	}
	_xclose(f);
}
