/* Message packet header. */

struct _hdr {
	int orig;		/* originating Node # */
	int dest;		/* destination node */
	int year,month,day,hour,minute,second;
	int rate;		/* baud rate */
	int extra[20];
};
