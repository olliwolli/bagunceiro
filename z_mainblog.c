#include <stdlib.h>
#include <unistd.h>
#include <str.h>
#include <textcode.h>

#include "z_blog.h"
#include "z_conf.h"
#include "z_time.h"
#include "z_features.h"

#define MAX_KEY_LENGTH_STR 50
#define MAX_FMT_LENGTH 5

#ifdef WANT_ERROR_PRINT
struct errors gerr = {
	.note = "",
	.error = 0,
	.type = N_NONE
};
#endif

/* DEBUGGING */
#ifdef DEBUG_PARSING
static array debugmsg;
static char daction[4][10] = { "Show", "Delete", "Add", "Modify" };

static void __d(const char *desc, const char *value)
{
	array_cats(&debugmsg, "<tr>");
	if (value) {
		array_cats(&debugmsg, "<th align=\"left\">");
		array_cats(&debugmsg, desc);
		array_cats(&debugmsg, "</th>");
		array_cats(&debugmsg, "<th align=\"left\">");
		array_cats(&debugmsg, value);
		array_cats(&debugmsg, "</th>");
	} else {
		array_cats(&debugmsg, "<th align=\"left\">");
		array_cats(&debugmsg, desc);
		array_cats(&debugmsg, "</th>");
		array_cats(&debugmsg, "<th align=\"left\">");
		array_cats(&debugmsg, "[Empty]<br>\n");
		array_cats(&debugmsg, "</th>");
	}
	array_cats(&debugmsg, "</tr>");
}

static void debug_print(const blog_t * conf, array * ps, array * qs)
{
	array_cats(&debugmsg, "<table>");
	__d("Poststring is: ", ps->p);
	__d("Querystring is: ", qs->p);
	__d("Cookie is: ", conf->cookie);
	__d("URL is: ", conf->script);
	__d("Host is: ", conf->host);
	__d("CSS arg is: ", conf->css);

	if (array_bytes(&conf->input))
		__d("Input is: ", conf->input.p);

	__d("Key is: ", conf->qry.ts);

	__d("Action is: ", daction[conf->qry.action]);
	array_cats(&debugmsg, "</table>");
}
#endif

#ifdef DEBUG_PARSE_POST
static void get_post_string(array * ps)
{
	memset(ps, 0, sizeof(array));
	array_cats0(ps, DEBUG_PARSE_POST);
}
#endif

#ifdef DEBUG_PARSE_QUERY
static void get_query_string(array * qs)
{
	memset(qs, 0, sizeof(array));
	array_cats0(qs, DEBUG_PARSE_QUERY);
}
#endif
#ifdef DEBUG_PARSE_COOKIE
static void get_cookie_string(array * co)
{
	memset(co, 0, sizeof(array));
	array_cats0(co, DEBUG_PARSE_COOKIE);
}
#endif

/* CONVERSION */
static int get_param_char(array * q_str, size_t qmax, char *search, char *ret,
	size_t n, char *sep)
{

	char *sep2 = "=";
	char *query, *tokptr;
	char *key, *val, *tokptr2;
	size_t destlen, qlen;

	qlen = array_bytes(q_str);

	char buf[qlen + 1];

	if (!qlen) {
		return -1;
	}

	if (qlen > qmax && qlen != -1) {
		return -1;
	}

	memcpy(buf, q_str->p, qlen);
	buf[qlen] = 0;

	for (query = strtok_r(buf, sep, &tokptr);
		query != NULL; query = strtok_r(NULL, sep, &tokptr)) {

		key = strtok_r(query, sep2, &tokptr2);
		val = strtok_r(NULL, sep2, &tokptr2);

		if (str_equal(key, search)) {
			/* just a key, no value */
			if (val == 0)
				return -2;

			/* not complying to length rules */
			if (str_len(val) >= n && n != -1) {
				str_copy(ret, "bad");
				set_err("Invalid input", 0, N_ERROR);
				return -3;
			}
			qlen = scan_urlencoded(val, ret, &destlen);
			ret[destlen] = 0; /* make it a string */
			return 0;	/* alright */
		}
	}
	return -1;
}

static void parse_cookie(blog_t * conf, array * co)
{
	int err;

	err = get_param_char(co, COOKIE_MAX, "css", conf->css, MAX_CSS_ARG,
		"; ");
	if (!err)
		conf->csstype = CSS_COOKIE;
}

static void parse_query(blog_t * conf, array * qs)
{
	int err;
	char buf[MAX_CSS_ARG+MAX_FMT_LENGTH+MAX_FMT_LENGTH];

	/* TIMESTAMP */
	err = get_param_char(qs, QUERY_MAX, "ts", conf->qry.ts,
		MAX_KEY_LENGTH_STR, "&");
	if (!err) {
		conf->qry.type = QRY_TS;
		conf->qry.action = QA_SHOW;
	}

	/* CSS STYLESHEET */
	err = get_param_char(qs, MAX_KEY_LENGTH_STR + 4, "css", conf->css,
		MAX_KEY_LENGTH_STR, "&");

	if(err == -2)
		conf->csstype = CSS_RESET;
	if(err == 0)
		conf->csstype = CSS_SELECT;

	/* FORMAT (HTML/RSS) */
	err = get_param_char(qs, MAX_FMT_LENGTH + 4, "fmt", buf,
		MAX_FMT_LENGTH, "&");
	if (err == 0 && str_equal(buf, "rss")) {
		conf->stype = S_RSS;
	}

	/* MONTH SELECTION */

	err = get_param_char(qs, QUERY_MAX, "mn", buf, FMT_CALDATE,
		"&");
	if (!err) {
		conf->qry.type = QRY_MONTH;
		conf->qry.action = QA_SHOW;
		/* append -01 (day), to make it compatible with conversion functions */
		strcat(buf, "-01");
		caldate_scan(buf, &conf->qry.mon.date);
	}else{
		caltime_utc(&conf->qry.mon, &conf->now.sec, (int *)0, (int *)0);
	}
}

/* keep that stuff in one function to not have ifdefs all around */
#ifdef ADMIN_MODE_PASS
static void do_admin_pass_mode(blog_t * conf, array * co, array * pd,
	array * qs)
{
	int err;
	char tmptoken[100];

	/* initialization */
	conf->authtype = AUTH_NONE;
	conf->ssl = SSL_OFF;

	strcpy(conf->sessionid, "");

	/* parse cookie */
	if(0 == get_param_char(co, COOKIE_MAX, "sid", conf->sessionid,
		SESSION_ID_LEN + 1, "; ")){
		conf->auth = validate_session_id(conf->sessionid);
		conf->authtype = AUTH_SID;
	}

	/*  parse postdata */

	err = get_param_char(pd, -1, "login", tmptoken, 100, "&");
	if (!err) {
		conf->auth = auth_conf(conf, (unsigned char*)tmptoken, strlen(tmptoken));
		memset(tmptoken, 0, 20);
		if(conf->auth)
			add_session_id(conf->sessionid);

		conf->authtype = AUTH_POST;
	}

// TODO streamline
	/* parse query string */
	err = get_param_char(qs, MAX_KEY_LENGTH_STR + 4, "login", conf->qry.ts,
		MAX_KEY_LENGTH_STR, "&");
	if (err == -2) {
		conf->qry.type = QRY_NONE;
		conf->qry.action = QA_LOGIN;
	}

	err = get_param_char(qs, MAX_KEY_LENGTH_STR + 4, "logout", conf->qry.ts,
		MAX_KEY_LENGTH_STR, "&");
	if (err == -2) {
		conf->qry.action = QA_LOGOUT;
		conf->auth = 0;
	}

	if (getenv("HTTPS") != NULL)
		conf->ssl = SSL_ON;

}
#endif

#ifdef ADMIN_MODE
static void do_admin_mode(blog_t * conf, array * co, array * pd, array * qs)
{
	/*  query */
	int err;
	char tmp[4];
	char *parg;

	err = get_param_char(qs, MAX_KEY_LENGTH_STR + 4, "del", conf->qry.ts,
		MAX_KEY_LENGTH_STR, "&");
	if (!err) {
		conf->qry.action = QA_DELETE;
		conf->qry.type = QRY_NONE;
	}

	err = get_param_char(qs, MAX_KEY_LENGTH_STR + 4, "add", NULL,
		MAX_KEY_LENGTH_STR, "&");
	if (err == -2){
		conf->qry.action = QA_ADD;
		conf->qry.type = QRY_NONE;
	}

#ifdef WANT_CGI_CONFIG
	err = get_param_char(qs, MAX_KEY_LENGTH_STR + 4, "config", NULL,
		MAX_KEY_LENGTH_STR, "&");
	if (err == -2){
		conf->qry.action = QA_CONFIG;
		conf->qry.type = QRY_NONE;
	}
#endif

	err = get_param_char(qs, MAX_KEY_LENGTH_STR + 4, "mod", conf->qry.ts,
		MAX_KEY_LENGTH_STR, "&");
	if (!err) {
		conf->qry.type = QRY_NONE;
		conf->qry.action = QA_MODIFY;
	}

	/*  postdata */
	/* TODO REVIEW IF POST DOES NOT WORK */
	if (array_bytes(pd) < POSTDATA_MAX && array_bytes(pd)) {
		parg = alloca(array_bytes(pd));
		if(get_param_char(pd, -1, "input", parg, -1, "&")==0){

			if (get_param_char(pd, -1, "action", tmp, 4, "&") == 0) {
				if (str_equal(tmp, "add"))
					conf->qry.action = QA_ADD;
				if (str_equal(tmp, "mod"))
					conf->qry.action = QA_MODIFY;

			}

			array_cats0(&conf->input, parg);
			get_param_char(pd, -1, "key", conf->qry.ts, MAX_KEY_LENGTH_STR,
				"&");
		}else if (get_param_char(pd, -1, "action", tmp, 7, "&") == 0){
			if (str_equal(tmp, "config"))
				conf->qry.action = QA_CONFIG;

			get_param_char(pd, -1, "title", conf->qry.title, MAX_CONF_STR, "&");
			get_param_char(pd, -1, "tagline", conf->qry.tagline, MAX_CONF_STR, "&");
			get_param_char(pd, -1, "input", conf->qry.pass, MAX_CONF_STR, "&");
		}
	}
}
#endif

#ifndef DEBUG_PARSE_QUERY
static void get_query_string(array * qs)
{
	memset(qs, 0, sizeof(array));
	if (getenv("QUERY_STRING") != NULL)
		array_cats0(qs, getenv("QUERY_STRING"));

}
#endif
#ifndef DEBUG_PARSE_POST
static void get_post_string(array * ps)
{
	char *len;
	memset(ps, 0, sizeof(array));

	len = getenv("CONTENT_LENGTH");
	if (len != 0) {
		while (buffer_get_array(buffer_0, ps)) {
			if (array_bytes(ps) > POSTDATA_MAX)
				break;
		}
	}
}
#endif
#ifndef DEBUG_PARSE_COOKIE
static void get_cookie_string(array * co)
{
	memset(co, 0, sizeof(array));
	if (getenv("HTTP_COOKIE") != NULL) {
		array_cats0(co, getenv("HTTP_COOKIE"));
	}
}
#endif

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
	array qs;		/* query string */
	array ps;		/* post string */
	array co;		/* cookie string */

	/* default configuration and parameters */
	static blog_t conf = {
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
				.type = QRY_WEEK,
				.start = 0,
				.doff = 8},
		.stype = S_HTML,
		.csstype = CSS_DEFAULT,
	};

#ifdef WANT_FAST_CGI
	while (FCGI_Accept() >= 0) {
#endif
		memset(&conf.input, 0, sizeof(array));
		memset(conf.qry.ts, 0, sizeof(conf.qry.ts));
		taia_now(&conf.now);

		gettimeofday(&conf.ptime, NULL);

		load_config(&conf);

		conf.host = getenv("SERVER_NAME");
		conf.script = getenv("SCRIPT_NAME");

		/* get the strings */
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

		if (conf.qry.type == QRY_TS && !verify_ts(conf.qry.ts)) {
			conf.qry.action = QA_SHOW;
			conf.qry.type = QRY_WEEK;
#ifdef WANT_ERROR_PRINT
			set_err("Not a valid timestamp", 0, N_ERROR);
#endif
		}

		/* set a cookie */
		if (conf.csstype != CSS_DEFAULT && conf.csstype != CSS_RESET) {
			if (access(conf.css, F_OK) == -1)
				conf.csstype = CSS_ERROR;
		}
		inflate_ts(conf.qry.ts);

		handle_query(&conf);

#ifdef DEBUG_PARSING
		debug_print(&conf, &ps, &qs);
		array_cat0(&debugmsg);
		sprintm("<div id=\"note\"><div id=\"head\">DEBUG</div>",
			debugmsg.p, "</div>");
		array_reset(&debugmsg);
#endif

		if (conf.stype != S_RSS)
			time_stop_print(&conf.ptime);
		else
			sprintf("");

#ifdef WANT_FAST_CGI
		array_reset(&qs);
		array_reset(&ps);
		array_reset(&co);
	}
#endif
	return 0;
}
