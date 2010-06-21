#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <array.h>
#include <ctype.h>
#include <caltime.h>
#include <errno.h>
#include <str.h>

#include "z_cdb.h"
#include "z_entry.h"
#include "z_time.h"
#include "z_blog.h"
#include "z_features.h"
#include "z_format.h"

/* Parses query and fetches data from database */
static int fetch_entry_ts(const blog_t * conf, array * blog)
{
	array *day_items;
	struct day_entry *day = malloc(sizeof(struct day_entry));
	struct caltime *day_time = malloc(sizeof(struct caltime));
	struct nentry e;

	memset(&e, 0, sizeof(e));

	scan_time_hex(conf->qry.ts, &e.k);
	show_entry(conf->db, &e);
	caltime_utc(day_time, &e.k.sec, (int *)0, (int *)0);

	day_items = e_malloc();
	e_add_to_array(&e, day_items);
	day->es = day_items;
	day->date = &day_time->date;

	array_catb(blog, (char *)day, sizeof(struct day_entry));

	return 0;
}

static int fetch_entries_days(const blog_t * conf, array * blog)
{
	int err, start, offset, i;
	struct day_entry *day;
	struct caltime *day_time;
	struct taia tday;
	array *day_items;

	start = conf->qry.start;
	offset = conf->qry.doff;
	taia_now(&tday);

	for(i=0; i< start; i++)
		ht_sub_days(&tday, 1);

	err = 0;

	for (start = 0; start < offset; start++) {
		/* allocate memory */
		day = malloc(sizeof(struct day_entry));
		day_items = malloc(sizeof(struct nentry));
		day_time = malloc(sizeof(struct caltime));
		day_items->allocated = 0;
		day_items->initialized = 0;
		memset(day_items, 0, sizeof(struct nentry));
		//TODO check if file exists in order to avoid mallocs above
		/* get entries for calculated day */
		err = show_day(conf->db, day_items, &tday);

		if (err <= 0)	/* File not found or 0 entries */
			goto sub;

		caltime_utc(day_time, &tday.sec, (int *)0, (int *)0);

		/* fill in the entries and date */
		day->es = day_items;
		day->date = &day_time->date;
		/* add the day to the blog */
		array_catb(blog, (char *)day, sizeof(struct day_entry));
		if (array_failed(blog)) {
			eprintf("Could not allocate memory!");
			exit(-1);
		}
sub:
		/* calculate day */
		ht_sub_days(&tday, 1);
	}

	return err;
}

#ifdef WANT_MONTH_BROWSING
/* TODO: optimize */
/* this calculates offsets from today and calls fetch_entries_days */
static int fetch_entries_month(blog_t *conf, array * blog)
{
	struct caltime ct;
	struct taia now;
	int err;
	long jnow, start, offset;

	err = 0;

	taia_now(&now);
	caltime_utc(&ct, &now.sec, (int *)0, (int *)0);
	jnow = caldate_mjd(&ct.date);

	caldate_scan(conf->qry.mon, &ct.date);

	/* go to the last day of the selected month */
	ct.date.month++;
	ct.hour = 1;
	ct.date.day--;

	start =	caldate_mjd(&ct.date);
	start = jnow - start;

	/* if the date is in the future reset to 0 (current month) */
	if(start <= 0)
		start = 0;

	ct.date.month--;
	ct.date.day = 1;
	offset = (jnow - caldate_mjd(&ct.date)) - start;

	conf->qry.start = start;
	conf->qry.doff = offset + 1; /* add one to also get first day of month */
	fetch_entries_days(conf, blog);

	return err;
}
#endif

int handle_query(blog_t * conf)
{
	array blog;
	struct nentry n;
	int err;

	memset(&n.e, 0, sizeof(array));
	memset(&blog, 0, sizeof(array));

	switch (conf->qry.action) {
	case QA_SHOW:
		switch (conf->qry.type) {
		case QRY_WEEK:
			err = fetch_entries_days(conf, &blog);
			break;
		case QRY_TS:
			fetch_entry_ts(conf, &blog);
			break;
#ifdef WANT_MONTH_BROWSING
		case QRY_MONTH:
			fetch_entries_month(conf, &blog);
			break;
#endif
		default:
			return -1;
		}
		print_show(&blog, conf);
		break;

#ifdef ADMIN_MODE
	case QA_ADD:
		/* show the add dialog */
		if (!array_bytes(&conf->input)) {
			print_add_entry(conf);
			/* add a new entry */
		} else {
			set_err("Entry added", 0, N_NOTE);
			array_cats0(&n.e, conf->input.p);
			add_entry_now(conf->db, &n);
			err = fetch_entries_days(conf, &blog);
			print_show(&blog, conf);
		}
		break;
	case QA_DELETE:
		scan_time_hex(conf->qry.ts, &n.k);
		err = delete_entry(conf->db, &n);
		if (err < 0) {
			set_err(conf->qry.ts, errno, N_ERROR);
		} else {
			set_err("Entry deleted", 0, N_NOTE);
		}
		fetch_entries_days(conf, &blog);
		print_show(&blog, conf);
		break;
	case QA_MODIFY:
		scan_time_hex(conf->qry.ts, &n.k);
		if (!array_bytes(&conf->input)) {
			print_mod_entry(conf, &n);
			/* add a new entry */
		} else {
			set_err("Entry modified", 0, N_NOTE);
			array_cats0(&n.e, conf->input.p);
			modify_entry(conf->db, &n);

			err = fetch_entries_days(conf, &blog);
			print_show(&blog, conf);
		}
		break;
#endif
#ifdef ADMIN_MODE_PASS
	case QA_LOGIN:
		print_login(conf);
		break;
	case QA_LOGOUT:
		print_show(&blog, conf);
		break;

#endif
	}
	/* TODO free day, day_items, blog */
	array_reset(&blog);
	array_reset(&n.e);
	return 0;
}
