#include <string.h>

#include <fmt.h>
#include <scan.h>
#include <tai.h>
#include <taia.h>
#include <array.h>
#include <textcode.h>
#include <str.h>
#include <caltime.h>

#include "z_features.h"
#include "z_time.h"

#ifdef WANT_REDUCE_TS
void reduce_ts(char *src)
{
	char tmp[FMT_TAIA_HEX];

	memset(tmp, '0', FMT_TAIA_HEX-1);
	memcpy(tmp, src + REDUCE_OFFSET, REDUCE_SIZE);
	memcpy(src, tmp, FMT_TAIA_HEX);
	src[REDUCE_SIZE] = 0;
}
/* We use 40 00 00 00 00 00 00 00 here
 * since we do not allow past timestamps
 * (more precisely timestamps before 1970)
 * for explanation see http://cr.yp.to/libtai/tai64.html#tai64 */
void inflate_ts(char *src)
{
	char tmp[FMT_TAIA_HEX];
	memset(tmp, '0', FMT_TAIA_HEX);
	memcpy(tmp + REDUCE_OFFSET, src, REDUCE_SIZE);
	memcpy(tmp, "40000000", REDUCE_OFFSET);

	memcpy(src, tmp, FMT_TAIA_HEX);
}
#else
void reduce_ts(char *src){}
void inflate_ts(char *src){}
#endif

/* the following format functions null-terminate the resulting strings */
size_t fmt_time_hex(char *s, const struct taia *time)
{
	char buf[TAIA_PACK + 1];
	int len;

	taia_pack(buf, time);
	len = fmt_hexdump(s, buf, TAIA_PACK);
	s[len] = '\0';
	return len;
}


#ifdef WANT_DT
char *day_long[5] = {
    "Sweetmorn", "Boomtime", "Pungenday", "Prickle-Prickle", "Setting Orange"
};

char *season_long[5] = {
    "Chaos", "Discord", "Confusion", "Bureaucracy", "The Aftermath"
};

char *holyday[5][2] = {
    "Mungday", "Chaoflux",
    "Mojoday", "Discoflux",
    "Syaday",  "Confuflux",
    "Zaraday", "Bureflux",
    "Maladay", "Afflux"
};

struct disc_time {
    int season; /* 0-4 */
    int day; /* 0-72 */
    int yday; /* 0-365 */
    int year; /* 3066- */
};

int cal[12] = {0,31,59,90,120,151,181,212,243,273,304,334};
int convert(struct disc_time * dt, const struct caldate * cd)
{
   dt->year = cd->year+1166;
   dt->day = cal[cd->month-1] + cd->day - 1;
   dt->season = 0;

   /* leap year */
   if ((dt->year % 4) == 2) {
	   if (dt->day == 59)
		   dt->day = -1;
	   else if (dt->day > 59)
		   dt->day -= 1;
   }

   dt->yday = dt->day;
   while (dt->day >= 73) {
	   dt->season++;
	   dt->day -= 73;
   }
   return 0;
}

unsigned int fmt_caldate_nav(char * s, const struct caldate * cd){

	struct disc_time dt;
	convert(&dt, cd);

	if(dt.day==-1){
		strcpy(s, "St. Tib's Day ");
		s += 14;
	}else{
		strcpy(s, day_long[dt.yday%5]);
		s += strlen(day_long[dt.yday%5]);
		strcpy(s, ", the ");
		s+= 6;

		s += fmt_uint(s, dt.day+1);

		switch ((dt.day+1)%10)
		{
		case 1:
			strcpy(s,"st");
			break;
		case 2:
			strcpy(s,"nd");
			break;
		case 3:
			strcpy(s,"rd");
			break;
		default:
			strcpy(s,"th");
		}
		s += 2;
		strcpy(s," day of ");
		s += 8;

		strcpy(s, season_long[dt.season]);
		s += strlen(season_long[dt.season]);

	}
	strcpy(s, " in the YOLD ");
	s += 13;

	s += fmt_uint(s, dt.year);
	return 111; /* FIXME */
}

void testit()
{
	struct caldate cd;
	cd.day = 3;
	cd.month = 2;
	cd.year = 2010;
	char s[128];
	fmt_caldate_nav(s, &cd);
	sprintf(s);
	int slen = strlen(s);
}
#else
char months[12][4] =
	{ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Okt",
	"Nov", "Dez"
};
char weekdays[7][4] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };

static int weekday(int month, int day, int year)
{
	int ix, tx, vx;
	vx = 0;
	switch (month) {
	case 2:
	case 6:
		vx = 0;
		break;
	case 8:
		vx = 4;
		break;
	case 10:
		vx = 8;
		break;
	case 9:
	case 12:
		vx = 12;
		break;
	case 3:
	case 11:
		vx = 16;
		break;
	case 1:
	case 5:
		vx = 20;
		break;
	case 4:
	case 7:
		vx = 24;
		break;
	}
	if (year > 1900)
		year -= 1900;
	ix = ((year - 21) % 28) + vx + (month > 2);

	tx = (ix + (ix / 4)) % 7 + day;
	return (tx % 7);
}

unsigned int fmt_caldate_nav(char *s, const struct caldate *cd)
{
	long x;
	int i = 0;
	int yi;

	x = cd->year;
	if (x < 0)
		x = -x;
	do {
		++i;
		x /= 10;
	} while (x);
	yi = i;
	if (s) {

		memcpy(s, weekdays[weekday(cd->month, cd->day, cd->year)], 3);
		s[3] = ' ';

		x = cd->day;
		s[5] = '0' + (x % 10);
		x /= 10;
		s[4] = '0' + (x % 10);
		if (s[4] == '0')
			s[4] = ' ';

		s[6] = ' ';
		x = cd->month;
		memcpy(s + 7, months[x-1], 3);
		s[10] = ' ';

		x = cd->year;

		s += yi + 11;
		do {
			*--s = '0' + (x % 10);
			x /= 10;
		} while (x);
	}

	return (cd->year < 0) + yi + 11;
}
#endif

size_t scan_time_hex(const char *s, struct taia * time)
{
	char buf[FMT_TAIA_HEX];
	size_t destlen = TAIA_PACK;
	if (!s)
		return 0;
	scan_hexdump(s, buf, &destlen);
	taia_unpack(buf, time);

	return TAIA_PACK;
}

#define DAYS_SECS 86400
size_t sub_days(struct taia * time, const unsigned int days)
{
	struct taia day;
	int i;

	taia_uint(&day, DAYS_SECS);
	for (i = 0; i < days; i++)
		taia_sub(time, time, &day);

	return days;
}

#undef DAYS_SECS

static int timeval_subtract(struct timeval *result, struct timeval *x,
	struct timeval *y)
{
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	   tv_usec  is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

void time_stop_print(struct timeval *time)
{
	struct timeval end, diff;

	char fmtsec[12];
	char fmtnsec[12];
	size_t len;

	gettimeofday(&end, NULL);
	timeval_subtract(&diff, &end, time);
	fmtsec[fmt_long(fmtsec, diff.tv_sec)] = 0;

	len = fmt_long(fmtnsec, diff.tv_usec);
	fmtnsec[len] = 0;

	sprintm("<!-- Execution took: ", fmtsec, ".");

	for (; len < 6; len++)
		sprint("0");

	sprintmf(fmtnsec, "s -->");
}

size_t fmt_time_str(char *s, const struct taia *time)
{
	struct caltime ct;
	size_t len;

	caltime_utc(&ct, &time->sec, (int *)0, (int *)0);
	len = caldate_fmt(s, &ct);
	s[len] = '\0';

	return len;
}

