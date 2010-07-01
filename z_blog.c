#include <stdlib.h>
#include <open.h>
#include <time.h>
#include <string.h>

#include <array.h>
#include <ctype.h>
#include <caltime.h>
#include <errno.h>
#include <str.h>
#include <cdb.h>

#include "z_cdb.h"
#include "z_entry.h"
#include "z_day_entry.h"
#include "z_time.h"
#include "z_blog.h"
#include "z_features.h"
#include "z_conf.h"
#include "z_format.h"

/* Parses query and fetches data from database */
static int fetch_entry_ts(const blog_t * conf, array * blog)
{
	struct day_entry *day = new_day_entry();
	struct nentry *e;
	struct cdb *result;

	if ((result = cdb_open_read(conf->db)) == NULL)
		return -1;

	e = new_nentry();
	scan_time_hex(conf->qry.ts, &e->k);

	_show_entry(result, e);

	caltime_utc(&day->time, &e->k.sec, (int *)0, (int *)0);

	array_catb(&day->es, (char *)e, sizeof(struct nentry));
	array_catb(blog, (char *)day, sizeof(struct day_entry));

	cdb_close(result);

	return 0;
}

static int fetch_entries_days(const blog_t * conf, array * blog)
{
	int num, start;
	struct day_entry *day;
	struct taia tday;
	struct cdb *result;
	num = 0;
	if ((result = cdb_open_read(conf->db)) == NULL)
		return -1;

	start = conf->qry.start;

	memcpy(&tday, &conf->now, sizeof(struct taia));

	ht_sub_days(&tday, start);

	for (start = 0; start < conf->qry.doff; start++) {
		/* allocate memory */
		day = new_day_entry();

		/* get entries for calculated day */
		if (_show_day(result, &day->es, &tday) <= 0) {	/* File not found or 0 entries */
			free_day_entry(day);
			goto sub;
		}

		caltime_utc(&day->time, &tday.sec, (int *)0, (int *)0);

		/* add the day to the blog */
		array_catb(blog, (char *)day, sizeof(struct day_entry));
		if (array_failed(blog)) {
			eprintf("Could not allocate memory!");
			exit(-1);
		}
		num++;
sub:
		/* calculate day */
		ht_sub_days(&tday, 1);
	}
	cdb_close(result);

	return num;
}

#ifdef WANT_MONTH_BROWSING
/* TODO: optimize */
/* this calculates offsets from today and calls fetch_entries_days */
static int fetch_entries_month(blog_t * conf, array * blog)
{
	struct caltime ct;
	int num;
	long jnow, start, offset;

	num = 0;
	caltime_utc(&ct, &conf->now.sec, (int *)0, (int *)0);
	jnow = caldate_mjd(&ct.date);

	memcpy(&ct, &conf->qry.mon, sizeof(struct caltime));
//	caldate_scan(conf->qry.mon, &ct.date);

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
	num = fetch_entries_days(conf, blog);

	return num;
}
#endif

int handle_query(blog_t * conf)
{
	array blog;
	struct nentry n;
	int err;

	memset(&n.e, 0, sizeof(array));
	memset(&blog, 0, sizeof(array));

#if defined(ADMIN_OVERRIDE) && defined(ADMIN_MODE)
	switch (conf->qry.action) {
	case QA_DELETE:;
	case QA_ADD:;

	case QA_MODIFY:
#ifdef WANT_CGI_CONFIG
	case QA_CONFIG:
#endif
		if (!conf->auth) {
			conf->qry.action = QA_SHOW;
			conf->qry.type = QRY_WEEK;
		}
		break;
	default:;
	}

#endif

#ifdef ADMIN_MODE_PASS
	if(conf->authtype == AUTH_POST && !conf->auth)
		set_err("Wrong password", 0, N_ERROR);
#endif

	switch (conf->qry.action) {
	case QA_SHOW:
		switch (conf->qry.type) {
		case QRY_WEEK:
			err = fetch_entries_days(conf, &blog);
			if(err < 1){
				fetch_entries_month(conf, &blog);
			}
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
		} else {
			set_err("Entry added", 0, N_NOTE);
			array_cats0(&n.e, conf->input.p);
			add_entry_now(conf->db, &n);
			err = fetch_entries_days(conf, &blog);
			conf->qry.action = QA_SHOW;
			print_show(&blog, conf);
		}
		break;

	case QA_DELETE:
		scan_time_hex(conf->qry.ts, &n.k);
		err = delete_entry(conf->db, &n);

		if (err < 0)
			set_err(conf->qry.ts, errno, N_ERROR);
		else
			set_err("Entry deleted", 0, N_NOTE);

		fetch_entries_days(conf, &blog);
		print_show(&blog, conf);
		break;

	case QA_MODIFY:
		scan_time_hex(conf->qry.ts, &n.k);
		if (!array_bytes(&conf->input)) {
			/* fetch entry from db, one could call fetch_entry_ts */
			struct cdb *result;
			if ((result = cdb_open_read(conf->db)) == NULL)
				return -1;
			_show_entry(result, &n);
			cdb_close(result);

			print_mod_entry(conf, &n);
			/* add a new entry */
		} else {
			set_err("Entry modified", 0, N_NOTE);
			array_cats0(&n.e, conf->input.p);
			modify_entry(conf->db, &n);

			err = fetch_entries_days(conf, &blog);
			conf->qry.action = QA_SHOW;
			print_show(&blog, conf);
		}
		break;
#endif
#ifdef ADMIN_MODE_PASS
	case QA_LOGIN:
		print_login(conf);
		break;
#ifdef WANT_CGI_CONFIG
	case QA_CONFIG:
		//FIXME
		if(!strlen(conf->qry.title) && !strlen(conf->qry.tagline) && !strlen(conf->qry.pass)){
			print_config(conf);
		}else{
			set_err("Configuration saved", 0, N_NONE);
			save_config(conf);
			print_show(&blog, conf);
		}
		break;
#endif
	case QA_LOGOUT:
		fetch_entries_days(conf, &blog);
		print_show(&blog, conf);
		break;

#endif
	}
	/* TODO free day, day_items, blog */
	array_reset(&blog);
	array_reset(&n.e);
	return 0;
}
