#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <array.h>
#include <buffer.h>
#include <str.h>
#include <textcode.h>

#include "z_http.h"
#include "z_blog.h"
#include "z_conf.h"
#include "z_time.h"
#include "z_features.h"

#ifdef DEBUG
#include "z_debug.h"
#endif

#ifdef WANT_FAST_CGI
#include "fcgiapp.h"
#endif

#define MAX_KEY_LENGTH_STR 50
#define MAX_FMT_LENGTH 5

#ifdef WANT_ERROR_PRINT
struct errors gerr = {
	.note = "",
	.error = 0,
	.type = N_NONE
};
#endif

/* CONVERSION */
#define P_UNLIMITED -1
#define P_NO_VALUE -2
#define RET_NO_VALUE -2
#define RET_INVALID -3
#define RET_NOT_FOUND -4
#define PARAM_SUCCESS 0

static int get_http_param(array * q_str, size_t qmax, char *search, char *ret,
	size_t n, char *sep)
{
	char sep2[2] = "=";
	char *query, *tokptr;
	char *key, *val, *tokptr2;
	size_t destlen, qlen;

	qlen = array_bytes(q_str);
	/* we write this array down here in order to use VLAs */
	char buf[qlen + 1];

	if (!qlen || (qlen > qmax && qlen != -1))
		return RET_INVALID;

	memcpy(buf, q_str->p, qlen);
	buf[qlen] = 0;

	for (query = strtok_r(buf, sep, &tokptr);
		query != NULL; query = strtok_r(NULL, sep, &tokptr)) {

		key = strtok_r(query, sep2, &tokptr2);
		val = strtok_r(NULL, sep2, &tokptr2);

		if (str_equal(key, search)) {
			/* just a key, no value */
			if (val == 0 || n == P_NO_VALUE)
				return RET_NO_VALUE;

			/* not complying to length rules */
			if ((strlen(val) > n && n != P_UNLIMITED)) {
				strcpy(ret, "bad");
				set_err("Invalid input", 0, N_ERROR);
				return RET_INVALID;
			}
			qlen = scan_urlencoded(val, ret, &destlen);
			ret[destlen] = 0;	/* make it a string */
			return PARAM_SUCCESS;	/* alright */
		}
	}
	return RET_NOT_FOUND;
}

static void parse_cookie(blog_t * conf, array * co)
{
	if (!get_http_param(co, COOKIE_MAX, QUERY_CSS, conf->qry.css,
			SIZE_HTTP_ARG, "; "))
		conf->qry.csstype = CSS_COOKIE;
}

static void parse_query(blog_t * conf, array * qs)
{
	int err;
	char buf[SIZE_HTTP_ARG + MAX_FMT_LENGTH + MAX_FMT_LENGTH];

	/* TIMESTAMP */
	err = get_http_param(qs, QUERY_MAX, QUERY_TS, conf->qry.ts,
		MAX_KEY_LENGTH_STR, "&");
	if (err == PARAM_SUCCESS) {
		conf->qry.type = QRY_TS;
		conf->qry.action = QA_SHOW;
	}
#ifdef WANT_SEARCHING
	err = get_http_param(qs, QUERY_MAX, QUERY_QRY, conf->qry.find,
		SIZE_FIND_STR, "&");
	if (err == PARAM_SUCCESS) {
		conf->qry.type = QRY_FIND;
		conf->qry.action = QA_SHOW;
	}
#endif

#ifdef WANT_TAGS
	err = get_http_param(qs, QUERY_MAX, QUERY_TAG, conf->qry.tag,
		SIZE_FIND_STR, "&");
	if (err == PARAM_SUCCESS) {
		conf->qry.type = QRY_TAG;
		conf->qry.action = QA_SHOW;
	}
#endif

	/* CSS STYLESHEET */
	err = get_http_param(qs, MAX_KEY_LENGTH_STR + 4, QUERY_CSS,
		conf->qry.css, MAX_KEY_LENGTH_STR, "&");

	if (err == RET_NO_VALUE)
		conf->qry.csstype = CSS_RESET;
	if (err == PARAM_SUCCESS)
		conf->qry.csstype = CSS_SELECT;

	/* FORMAT (HTML/RSS) */
	err = get_http_param(qs, MAX_FMT_LENGTH + 4, QUERY_FMT, buf,
		MAX_FMT_LENGTH, "&");
	if (err == PARAM_SUCCESS && str_equal(buf, "rss")) {
		conf->qry.stype = S_RSS;
	}

	/* MONTH SELECTION */
	err = get_http_param(qs, QUERY_MAX, QUERY_MONTH, buf, FMT_CALDATE, "&");
	if (err == PARAM_SUCCESS) {
		conf->qry.type = QRY_MONTH;
		conf->qry.action = QA_SHOW;
		/* append -01 (day), to make it compatible with conversion functions */
		strcat(buf, "-01");
		caldate_scan(buf, &conf->qry.mon.date);
	} else {
		caltime_utc(&conf->qry.mon, &conf->now.sec, (int *)0, (int *)0);
	}
}

/* keep that stuff in one function to not have ifdefs all around */
#ifdef ADMIN_MODE_PASS
static void do_admin_pass_mode(blog_t * conf, array * co, array * pd,
	array * qs)
{
	int err;
	char tmptoken[10];

	/* initialization */
	conf->authtype = AUTH_NONE;
	conf->auth = 0;
	strcpy(conf->sid, "");

	/* parse cookie */
	if (PARAM_SUCCESS == get_http_param(co, COOKIE_MAX, COOKIE_SID,
			conf->sid, SIZE_SESSION_ID + 1, "; ")) {
		conf->auth = validate_session_id(conf->sid);
		conf->authtype = AUTH_SID;
	}

	/*  parse postdata */
	err = get_http_param(pd, P_UNLIMITED, POST_LOGIN, tmptoken, 100, "&");
	if (err == PARAM_SUCCESS) {
		conf->auth =
			auth_conf(conf, (unsigned char *)tmptoken,
			strlen(tmptoken));
		memset(tmptoken, 0, sizeof(tmptoken));
		if (conf->auth) {
			err = add_session_id(conf->sid);
			if (err < 0)
				set_err("Could not add session id, check if the db/ directory is writable by the webserver", 0, N_ERROR);
		}

		conf->authtype = AUTH_POST;
	}

	/* parse query string */
	err = get_http_param(qs, MAX_KEY_LENGTH_STR + 4, QUERY_LOGIN,
		conf->qry.ts, P_NO_VALUE, "&");
	if (err == RET_NO_VALUE) {
		conf->qry.type = QRY_NONE;
		conf->qry.action = QA_LOGIN;
	}

	err = get_http_param(qs, MAX_KEY_LENGTH_STR + 4, QUERY_LOGOUT,
		conf->qry.ts, P_NO_VALUE, "&");
	if (err == RET_NO_VALUE) {
		conf->qry.action = QA_LOGOUT;
		conf->auth = 0;
	}
}
#endif

#ifdef ADMIN_MODE
static void do_admin_mode(blog_t * conf, array * co, array * pd, array * qs)
{
	/*  query */
	int err;
	char parg[array_bytes(pd)];
	char tmp[30];

	err = get_http_param(qs, MAX_KEY_LENGTH_STR + 4, QUERY_DEL,
		conf->qry.ts, MAX_KEY_LENGTH_STR, "&");
	if (err == PARAM_SUCCESS) {
		conf->qry.action = QA_DELETE;
		conf->qry.type = QRY_NONE;
	}

	err = get_http_param(qs, MAX_KEY_LENGTH_STR + 4, QUERY_ADD, NULL,
		P_NO_VALUE, "&");
	if (err == RET_NO_VALUE) {
		conf->qry.action = QA_ADD;
		conf->qry.type = QRY_NONE;
	}
#ifdef WANT_CGI_CONFIG
	err = get_http_param(qs, MAX_KEY_LENGTH_STR + 4, QUERY_CONFIG, NULL,
		P_NO_VALUE, "&");
	if (err == RET_NO_VALUE) {
		conf->qry.action = QA_CONFIG;
		conf->qry.type = QRY_NONE;
	}
#endif

	err = get_http_param(qs, MAX_KEY_LENGTH_STR + 4, QUERY_MOD,
		conf->qry.ts, MAX_KEY_LENGTH_STR, "&");
	if (err == PARAM_SUCCESS) {
		conf->qry.type = QRY_NONE;
		conf->qry.action = QA_MODIFY;
	}

	/*  postdata */
	if (array_bytes(pd) < POSTDATA_MAX && array_bytes(pd)) {

		/* FIXME: 7 should be calculated in some way here to avoid future errors */
		if (get_http_param(pd, P_UNLIMITED, POST_ACTION, tmp, 7,
				"&") == PARAM_SUCCESS) {

			if (str_equal(tmp, POST_ACTION_ADD))
				conf->qry.action = QA_ADD_POST;
			else if (str_equal(tmp, POST_ACTION_MOD))
				conf->qry.action = QA_MODIFY_POST;
			else if (str_equal(tmp, POST_ACTION_CONFIG)) {
				conf->qry.action = QA_CONFIG_POST;
				get_http_param(pd, P_UNLIMITED, POST_ARG_TITLE,
					conf->qry.title, SIZE_HTTP_ARG, "&");
				get_http_param(pd, P_UNLIMITED,
					POST_ARG_TAGLINE, conf->qry.tagline,
					SIZE_TAGLINE, "&");
				get_http_param(pd, P_UNLIMITED, POST_ARG_PASS,
					conf->qry.pass, SIZE_HTTP_ARG, "&");
				if (get_http_param(pd, P_UNLIMITED,
						POST_ARG_SEARCHBOX, &conf->sbox,
						1, "&") == RET_NOT_FOUND)
					conf->sbox = 'n';
			}

			if (conf->qry.action == QA_ADD_POST
				|| conf->qry.action == QA_MODIFY_POST) {
				err = get_http_param(pd, P_UNLIMITED,
					POST_ARG_INPUT, parg, P_UNLIMITED, "&");
				if (err == PARAM_SUCCESS) {
					array_cats0(&conf->qry.input, parg);
					get_http_param(pd, P_UNLIMITED,
						POST_ARG_KEY, conf->qry.ts,
						MAX_KEY_LENGTH_STR, "&");
				}
			}
		}
	}
}
#endif

/* the method of getting the environment variables depends on the use
 * of cgi or fcgi*/

static void get_query_string(array * qs)
{
	memset(qs, 0, sizeof(array));

#ifdef DEBUG
	array_cats0(qs, debug_query);
	return;
#endif

	if (getenv("QUERY_STRING") != NULL)
		array_cats0(qs, getenv("QUERY_STRING"));
}

static void get_cookie_string(array * co)
{
	memset(co, 0, sizeof(array));

#ifdef DEBUG
	array_cats0(co, debug_cookie);
	return;
#endif

	if (getenv("HTTP_COOKIE") != NULL)
		array_cats0(co, getenv("HTTP_COOKIE"));
}

static void get_post_string(array * ps)
{
	char *len;

	memset(ps, 0, sizeof(array));

#ifdef DEBUG
	array_cats0(ps, debug_post);
	return;
#endif

	len = getenv("CONTENT_LENGTH");
	if (len != 0) {

#ifdef WANT_FAST_CGI
		char ch[128];
		int r;
		while ((r = FCGX_GetStr(ch, 128, fcgi_in))) {
			array_catb(ps, ch, r);
			if (r < 128)
				break;
		}
#else
		while (buffer_get_array(buffer_0, ps) > 0) {
			if (array_bytes(ps) > POSTDATA_MAX)
				break;
		}
#endif
	}
}

static void make_host_string(blog_t * conf)
{
	int hlen, slen;

	hlen = slen = 0;

	if (conf->host != NULL)
		hlen = strlen(conf->host);

	if (conf->script != NULL)
		slen = strlen(conf->script);

	if (!hlen || !slen || hlen + slen + PROTO_HTTPS_LEN + 1 > URL_PATH) {
		strcpy(conf->path, "/");
		return;
	}

	if (conf->ssl != NULL)
		strcpy(conf->path, PROTO_HTTPS);
	else
		strcpy(conf->path, PROTO_HTTP);

	strcat(conf->path, conf->host);
	if (slen)
		strcat(conf->path, conf->script);
}

static int verify_ts(char *ts)
{
	int i;
#ifdef WANT_REDUCE_TS
	for (i = 0; i < REDUCE_SIZE; i++) {
#else
	for (i = 0; i < FMT_TAIA_HEX; i++) {
#endif
		if (ts[i] < '0' || (ts[i] > '9' && ts[i] < 'a') || ts[i] > 'f')
			return 0;
	}
	return 1;
}

int main()
{
	int err;
	array qs;		/* query string */
	array ps;		/* post string */
	array co;		/* cookie string */

	/* one time initialization */
	blog_t conf = {
		.title = "",
		.tagline = "",
		.db = DB_FILE,

		.qry = {
				.ts = "",
#if defined(WANT_CGI_CONFIG) && defined(ADMIN_MODE)
				.title = "",
				.tagline = "",
				.pass = "",
#endif
			},
		.sbox = 'n'
	};

//      testit();

	/* load the configuration, only done once. It is save_configs
	 * responsibility to update the on disk and in memory values
	 * when they are changed */
	err = load_config(&conf);
	if (err < 0)
		set_err("Could not load config", 0, N_ERROR);

#if defined(WANT_FAST_CGI) && !defined(DEBUG_MEMORY)
	while (FCGX_Accept(&fcgi_in, &fcgi_out, &fcgi_err, &envp) >= 0) {
#endif

#ifdef DEBUG_MEMORY
		int i = DEBUG_MEMORY;
		while (i--) {
#endif

			/* reset variables */
			if (getenv("SERVER_NAME"))
				strncpy(conf.host, getenv("SERVER_NAME"),
					MAX_HOST_SIZE);
			else
				strcpy(conf.host, "");
			conf.host[MAX_HOST_SIZE - 1] = 0;

			if (getenv("SCRIPT_NAME"))
				strncpy(conf.script, getenv("SCRIPT_NAME"),
					MAX_SCRIPT_SIZE);
			else
				strcpy(conf.script, "/");
			conf.script[MAX_SCRIPT_SIZE - 1] = 0;

			conf.mod = getenv("HTTP_IF_MODIFIED_SINCE");
			conf.ssl = getenv("HTTPS");

			conf.qry.action = QA_SHOW;
			conf.qry.type = QRY_WEEK;
			conf.qry.start = 0;
#ifdef WANT_ERISIAN_CALENDAR
			/*. The days of the week are named after
			 *  the five basic Discordian elements,
			 *  Sweet, Boom, Pungent, Prickle, and Orange.
			 *   There are 73 of these weeks per year. */
			conf.qry.doff = 6;
#else
			conf.qry.doff = 8;
#endif
			conf.qry.stype = S_HTML;
			conf.qry.csstype = CSS_DEFAULT;

			memset(&conf.qry.input, 0, sizeof(array));
			memset(conf.qry.ts, 0, sizeof(conf.qry.ts));

			/* logic */
			taia_now(&conf.now);
			gettimeofday(&conf.ptime, NULL);
			make_host_string(&conf);

			/* get the cgi variables */
			get_cookie_string(&co);
			get_post_string(&ps);
			get_query_string(&qs);

			/* parse cookie first */
			parse_cookie(&conf, &co);
			parse_query(&conf, &qs);

#ifdef ADMIN_MODE
			do_admin_mode(&conf, &co, &ps, &qs);
#endif

#ifdef ADMIN_MODE_PASS
			do_admin_pass_mode(&conf, &co, &ps, &qs);
#endif

			/* verify the timestamp */
			if (conf.qry.type == QRY_TS && !verify_ts(conf.qry.ts)) {
				conf.qry.action = QA_SHOW;
				conf.qry.type = QRY_WEEK;
#ifdef WANT_ERROR_PRINT
				set_err("Not a valid timestamp", 0, N_ERROR);
#endif
			}

			/* verify if the select css stylesheet exists */
			if (conf.qry.csstype != CSS_DEFAULT
				&& conf.qry.csstype != CSS_RESET) {
				if (access(conf.qry.css, F_OK) == -1)
					conf.qry.csstype = CSS_ERROR;
			}
			inflate_ts(conf.qry.ts);

			/* configuration and parameter parsing done. do something */
			err = handle_query(&conf);

#ifdef DEBUG_PARSING
			debug_print(&conf, &ps, &qs, &co);
			array_cat0(&debugmsg);
			sprintm("<div id=\"note\"><div id=\"head\">DEBUG</div>",
				debugmsg.p, "</div>");
			array_reset(&debugmsg);
#endif

			if (!err && conf.qry.stype != S_RSS)
				time_stop_print(&conf.ptime);
			else
				sprintf("");

			array_trunc(&conf.qry.input);

#if defined(WANT_FAST_CGI) || defined(DEBUG_MEMORY)
			array_trunc(&qs);
			array_trunc(&ps);
			array_trunc(&co);
		}
#endif

		return 0;
	}
