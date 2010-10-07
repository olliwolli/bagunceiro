#include <stdlib.h>
#include <open.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <array.h>
#include <caltime.h>
#include <str.h>
#include <cdb.h>

#include "z_features.h"
#include "z_result.h"
#include "z_entry.h"
#include "z_day.h"
#include "z_cdbb.h"
#include "z_time.h"
#include "z_blog.h"
#include "z_conf.h"
#include "z_format.h"

#if defined(WANT_SEARCHING) || defined(WANT_TAGS)
static void add_to_result(struct result *res, struct nentry *n)
{
	struct taia tday;
	struct caltime ttime;
	struct day *day = NULL;
	int i;
	int found = 0;

	taia_unpack(n->k.p, &tday);
	caltime_utc(&ttime, &tday.sec, (int *)0, (int *)0);

	for (i = 0; i < result_length(res); i++) {
		day = result_get_day_entry(res, i);
		if (day != NULL && day->time.date.day == ttime.date.day &&
			day->time.date.month == ttime.date.month &&
			day->time.date.year == ttime.date.year) {
			found = 1;
			break;
		} else {
			day = NULL;
		}
	}

	if (day == NULL)
		day = day_new();

	/* entries to day */
	memcpy(&day->time, &ttime, sizeof(struct caltime));
	day_add_nentry(day, n);

	/* day to blog */
	if (!found)
		result_add_day_entry(res, day);
}

static int datecomp(const void *a, const void *b)
{
	struct day *A = (struct day *)a;
	struct day *B = (struct day *)b;

	int l;
	l = B->time.date.year - A->time.date.year;
	if (l != 0)
		return l;
	l = B->time.date.month - A->time.date.month;
	if (l != 0)
		return l;
	l = B->time.date.day - A->time.date.day;
	return l;
}

static int __fetch_entry_find(const blog_t * conf, struct result *res,
	int ignorecase)
{
	struct cdbb a;
	struct nentry *n;

	int num, err;

	num = 0;
	cdbb_open_read(&a, conf->db);
	if (cdbb_firstkey(&a) == -1)
		return -1;

	while (42) {
		n = new_nentry();
		if (n == NULL)
			continue;

		if (ignorecase)
			err = cdbb_findnext_ignorecase(&a, conf->qry.find, n);
		else
			err = cdbb_findnext(&a, conf->qry.find, n);

		if (err == -1) {
			free_nentry(n);
			return num;
		} else if (err == 0) {
			free_nentry(n);
			continue;
		}

		add_to_result(res, n);
		num++;
	}
}
#endif

#ifdef WANT_SEARCHING
static int fetch_entry_query(const blog_t * conf, struct result *res)
{
	return __fetch_entry_find(conf, res, 0);
}
#endif

#ifdef WANT_TAGS
static int fetch_entry_tag(const blog_t * conf, struct result *res)
{
	return __fetch_entry_find(conf, res, 1);
}
#endif

/* Parses query and fetches data from database */
static int fetch_entry_ts(const blog_t * conf, struct result *res)
{
	struct day *day = day_new();
	struct nentry *e;
	struct taia k;
	struct cdbb a;

	if (cdbb_open_read(&a, conf->db) <= 0)
		return -1;

	e = new_nentry();
	if (e == NULL)
		goto err;

	scan_time_hex(conf->qry.ts, &k);
	show_entry(&a, &k, e);

	caltime_utc(&day->time, &k.sec, (int *)0, (int *)0);

	day_add_nentry(day, e);
	result_add_day_entry(res, day);

err:
	cdbb_close_read(&a);
	return 0;
}

static int fetch_entries_days(const blog_t * conf, struct result *res)
{
	int num, start;
	struct day *day;
	struct taia tday;
	struct cdbb a;
	num = 0;

	if (cdbb_open_read(&a, conf->db) <= 0)
		return -1;

	start = conf->qry.start;

	memcpy(&tday, &conf->now, sizeof(struct taia));
	sub_days(&tday, start);

	for (start=0 ; start < conf->qry.doff; start++) {
		day = day_new();

//		caltime_utc(&day->time, &tday.sec, (int *)0, (int *)0);
//		printf("Today is: %d.%d\n", day->time.date.day, day->time.date.month);

		/* get entries for calculated day */
		if (cdbb_fetch_day(&a, day, &tday) <= 0) {	/* File not found or 0 entries */
			day_free(day);
			goto sub;
		}

		caltime_utc(&day->time, &tday.sec, (int *)0, (int *)0);

		/* add the day to the blog */
		result_add_day_entry(res, day);
//              if (array_failed(blog)) {FIXME
//                      eprintf("Could not allocate memory!");
//                      return -1;
//              }
		num++;
sub:
	;
		/* calculate day */
		sub_days(&tday, 1);
	}
	cdbb_close_read(&a);

	return num;
}

#ifdef WANT_MONTH_BROWSING
#ifdef WANT_ERISIAN_CALENDAR
static int fetch_entries_month(blog_t * conf, struct result *res)
{
	// TODO:
}
#endif

/* this calculates offsets from today and calls fetch_entries_days */
static int fetch_entries_month(blog_t * conf, struct result *res)
{
	struct caltime ct;
	int num;
	long jnow, start, offset;

	num = 0;
	caltime_utc(&ct, &conf->now.sec, (int *)0, (int *)0);
	jnow = caldate_mjd(&ct.date);

	memcpy(&ct, &conf->qry.mon, sizeof(struct caltime));
//      caldate_scan(conf->qry.mon, &ct.date);

	/* go to the last day of the selected month */
	ct.date.month++;
	ct.hour = 1;
	ct.date.day--;

	start = caldate_mjd(&ct.date);
	start = jnow - start;

	/* if the date is in the future reset to 0 (current month) */
	if (start <= 0)
		start = 0;

	ct.date.month--;
	ct.date.day = 1;
	offset = caldate_mjd(&ct.date);
	offset = (jnow - offset) - start;

	/* too far in future (next month) */
	if (offset <= 0)
		offset = -1;

	conf->qry.start = start;
	conf->qry.doff = offset + 1;	/* add one to also get first day of month */
	num = fetch_entries_days(conf, res);

	return num;
}
#endif

ssize_t buffer_get_array(buffer * b, array * x)
{
	unsigned long done;
	int blen;
	done = 0;
	while (42) {
		if ((blen = buffer_feed(b)) <= 0)
			return blen;
		array_catb(x, b->x + b->p, blen);
		b->p += blen;
		done += blen;
	}
	return done;
}

int handle_query(blog_t * conf)
{
	struct result res;
	struct nentry n;
#ifdef ADMIN_MODE
	struct cdbb a;
#endif
	struct taia k;
	int err = 0;

	init_nentry(&n);
	result_init(&res);

	scan_time_hex(conf->qry.ts, &k);

#if defined(ADMIN_MODE)
	if (IS_ADMIN_ACTION(conf->qry.action) && !conf->auth) {
		conf->qry.action = QA_SHOW;
		conf->qry.type = QRY_WEEK;
	}
#endif

#ifdef ADMIN_MODE_PASS
	if (conf->authtype == AUTH_POST && !conf->auth)
		set_err("Wrong password", 0, N_ERROR);
#endif

	switch (conf->qry.action) {
	case QA_SHOW:
		switch (conf->qry.type) {
		case QRY_WEEK:
			/* here we try to get a week full of blog posts. (7days)
			 * If not all 7 days have blog posts. We get another week
			 * and if that one does not fill the posts up to 7, we get
			 * another one, and so on */
			err = 0;
			int i = 100;
			do {
				// FIXME fetch_entries_days opens and closes database all the time */
				err += fetch_entries_days(conf, &res);

				/* if we did get end-start (number of days) entries -> ok */
				if (err + 1 >= conf->qry.doff)
					break;

			/* now, this might actually get more than doff, entries,
			 * but whatever -- I hereby declare that doff has that meaning
			 * at least when used with QRY_WEEK */
				conf->qry.start = conf->qry.start + conf->qry.doff;
				conf->qry.doff = 7;

			} while (i--);

			break;
		case QRY_TS:
			fetch_entry_ts(conf, &res);
			break;
#ifdef WANT_SEARCHING
		case QRY_FIND:
			fetch_entry_query(conf, &res);

			qsort(res.r.p,
				result_length(&res),
				sizeof(struct day **), datecomp);
			break;
#endif
#ifdef WANT_TAGS
		case QRY_TAG:
			fetch_entry_tag(conf, &res);

			qsort(res.r.p,
				result_length(&res),
				sizeof(struct day **), datecomp);
			break;
#endif
#ifdef WANT_MONTH_BROWSING
		case QRY_MONTH:
			fetch_entries_month(conf, &res);
			break;
#endif
		default:
			return -1;
		}
		err = print_show(&res, conf);
		break;
#ifdef ADMIN_MODE
	case QA_ADD:
		/* show the add dialog */
		print_add_entry(conf);
		break;
	case QA_ADD_POST:
		err = cdbb_start_mod(&a, conf->db);
		if (err <= 0) {
			set_err("Could not open database files", 0, N_ERROR);
		} else {

			set_err("Entry added", 0, N_NOTE);
			add_entry_now(&a, conf->qry.input.p,
				array_bytes(&conf->qry.input));
			cdbb_apply(&a);
		}
		err = fetch_entries_days(conf, &res);
		conf->qry.action = QA_SHOW;
		print_show(&res, conf);
		break;
	case QA_DELETE:
		err = cdbb_start_mod(&a, conf->db);
		if (err <= 0) {
			set_err("Could not open database files", 0, N_ERROR);
		} else {
			delete_entry(&a, &k);
			err = cdbb_apply(&a);

			if (err < 0)
				set_err(conf->qry.ts, errno, N_ERROR);
			else
				set_err("Entry deleted", 0, N_NOTE);
		}
		fetch_entries_days(conf, &res);
		print_show(&res, conf);
		break;

	case QA_MODIFY:
		if (cdbb_open_read(&a, conf->db) > 0)
			show_entry(&a, &k, &n);
		cdbb_close_read(&a);

		print_mod_entry(conf, &n);
		/* add a new entry */
		break;
	case QA_MODIFY_POST:
		err = cdbb_start_mod(&a, conf->db);
		if (err <= 0) {
			set_err("Could not open database files", 0, N_ERROR);
		} else {
			set_err("Entry modified", 0, N_NOTE);
			modify_entry(&a, &k, conf->qry.input.p,
				array_bytes(&conf->qry.input));
			cdbb_apply(&a);
		}

		err = fetch_entries_days(conf, &res);
		conf->qry.action = QA_SHOW;
		print_show(&res, conf);
		break;
#endif
#ifdef ADMIN_MODE_PASS
#ifdef WANT_CGI_CONFIG
	case QA_CONFIG:
		print_config(conf);
		break;
	case QA_CONFIG_POST:
		set_err("Configuration saved", 0, N_NONE);
		save_config(conf);
		err = fetch_entries_days(conf, &res);
		conf->qry.action = QA_SHOW;
		print_show(&res, conf);
		break;
#endif
	case QA_LOGIN:
		print_login(conf);
		break;
	case QA_LOGOUT:
		err = expire_all_sessions(conf);
		if (err < 0)
			set_err("Could not expire sessions", 0, N_ERROR);

		fetch_entries_days(conf, &res);
		err = print_show(&res, conf);
		break;
#endif
	}

	result_reset(&res);
	reset_nentry(&n);
	return err;
}
