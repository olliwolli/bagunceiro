#include <stdlib.h>
#include <unistd.h>
#include <byte.h>
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
	byte_zero(ps, sizeof(array));
	array_cats0(ps, "log=sdfds");
}
#endif

#ifdef DEBUG_PARSE_QUERY
static void get_query_string(array * qs)
{
	byte_zero(qs, sizeof(array));
	array_cats0(qs, "css=style.css");
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

	byte_copy(buf, qlen, q_str->p);
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
				return -3;
			}
			qlen = scan_urlencoded(val, ret, &destlen);
			ret[qlen] = 0;
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
	char tmpcss[MAX_CSS_ARG];
	char type[MAX_FMT_LENGTH];

	err = get_param_char(qs, QUERY_MAX, "ts", conf->qry.ts,
		MAX_KEY_LENGTH_STR, "&");
	if (!err) {
		conf->qry.type = QRY_TS;
		conf->qry.action = QA_SHOW;
	}

	err = get_param_char(qs, MAX_KEY_LENGTH_STR + 4, "css", tmpcss,
		MAX_KEY_LENGTH_STR, "&");

	switch (err) {
		/* not found */
	case -1:
		break;
		/* no argument */
	case -2:
		conf->csstype = CSS_RESET;
		break;
	default:
		conf->csstype = CSS_SELECT;
		str_copy(conf->css, tmpcss);
		break;
	}

	err = get_param_char(qs, MAX_FMT_LENGTH + 4, "fmt", type,
		MAX_FMT_LENGTH, "&");
	if (err == 0 && str_equal(type, "rss")) {
		conf->stype = S_RSS;
	}
}

/* keep that stuff in one function to not have ifdefs all around */
#ifdef ADMIN_MODE_PASS
static void do_admin_pass_mode(blog_t * conf, array * co, array * pd, array * qs)
{
	int err;

	/* initialization */
	conf->authcache = 0;
	conf->authpost = 0;
	conf->ssl = 0;
	str_copy((char *)conf->token, "");

	/* parse cookie */
	char buf[SHA256_DIGEST_LENGTH * 2 + 1];
	size_t len;
	err = get_param_char(co, COOKIE_MAX, "token", buf,
		SHA256_DIGEST_LENGTH * 2 + 1, "; ");
	scan_hexdump(buf, (char *)conf->token, &len);

	/*  parse postdata */
	char tmptoken[SHA256_DIGEST_LENGTH * 2 + 1];
	err = get_param_char(pd, -1, "login", tmptoken,
		SHA256_DIGEST_LENGTH * 2 + 1, "&");
	if (!err) {
		conf->authpost = 1;
		SHA256((unsigned char *)tmptoken, strlen(tmptoken),
			conf->token);
		byte_zero(tmptoken, SHA256_DIGEST_LENGTH * 2);
	}

	/* parse query string */
	err = get_param_char(qs, MAX_KEY_LENGTH_STR + 4, "login", conf->qry.ts,
		MAX_KEY_LENGTH_STR, "&");
	if (err == -2) {
		conf->qry.type = QRY_TS;
		conf->qry.action = QA_LOGIN;
	}

	err = get_param_char(qs, MAX_KEY_LENGTH_STR + 4, "logout", conf->qry.ts,
		MAX_KEY_LENGTH_STR, "&");
	if (err == -2) {
		conf->qry.action = QA_LOGOUT;
	}

	/* check ssl status */
	if (getenv("HTTPS") != NULL)
		conf->ssl = 1;

	conf->authcache = auth_conf(conf, conf->token, SHA256_DIGEST_LENGTH);

}
#endif

#ifdef ADMIN_MODE
static void do_admin_mode(blog_t *conf, array * co, array * pd,  array * qs)
{
	/*  query */
	int err;
	char tmp[4];
	char *parg;

	err = get_param_char(qs, MAX_KEY_LENGTH_STR + 4, "del", conf->qry.ts,
		MAX_KEY_LENGTH_STR, "&");
	if (!err) {
		conf->qry.type = QRY_TS;
		conf->qry.action = QA_DELETE;
	}

	err = get_param_char(qs, MAX_KEY_LENGTH_STR + 4, "add", NULL,
		MAX_KEY_LENGTH_STR, "&");
	if (err == -2)
		conf->qry.action = QA_ADD;

	err = get_param_char(qs, MAX_KEY_LENGTH_STR + 4, "mod", conf->qry.ts,
		MAX_KEY_LENGTH_STR, "&");
	if (!err) {
		conf->qry.type = QRY_TS;
		conf->qry.action = QA_MODIFY;
	}

	/*  postdata */
	if (array_bytes(pd) < POSTDATA_MAX && array_bytes(pd)) {
		parg = alloca(array_bytes(pd));
		get_param_char(pd, -1, "input", parg, -1, "&");
		/*  TODO chek for error */
		array_cats0(&conf->input, parg);

		get_param_char(pd, -1, "key", conf->qry.ts, MAX_KEY_LENGTH_STR,
			"&");

		if (get_param_char(pd, -1, "action", tmp, 4, "&") == 0) {
			if (str_equal(tmp, "add"))
				conf->qry.action = QA_ADD;
			if (str_equal(tmp, "mod"))
				conf->qry.action = QA_MODIFY;
		}
	}
}
#endif

#ifndef DEBUG_PARSE_QUERY
static void get_query_string(array * qs)
{
	byte_zero(qs, sizeof(array));
	if (getenv("QUERY_STRING") != NULL)
		array_cats0(qs, getenv("QUERY_STRING"));

}
#endif
#ifndef DEBUG_PARSE_POST
static void get_post_string(array * ps)
{
	char *len;
	byte_zero(ps, sizeof(array));

	len = getenv("CONTENT_LENGTH");
	if (len != 0) {
		while (buffer_get_array(buffer_0, ps)) {
			if (array_bytes(ps) > POSTDATA_MAX)
				break;
		}
	}
}
#endif

static void get_cookie_string(array * co)
{
	byte_zero(co, sizeof(array));
	if (getenv("HTTP_COOKIE") != NULL) {
		array_cats0(co, getenv("HTTP_COOKIE"));
	}
}

static int verify_ts(char * ts)
{
	int i;
	for(i = 0; i < FMT_TAIA_HEX; i++){
		if( ts[i] < '0' || (ts[i] > '9' && ts[i] < 'a') || ts[i] > 'f')
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
		.title = "Blog",
		.db = "db/",
		.qry = {
				.ts = "",
				.type = QRY_WEEK,
				.start = 0,
			.end = 8},
		.stype = S_HTML,
		.cookie = NULL,
		.csstype = CSS_DEFAULT,
	};
	byte_zero(&conf.input, sizeof(array));

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

	if(conf.qry.type == QRY_TS && !verify_ts(conf.qry.ts) ){
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

	time_stop_print(&conf.ptime);

	array_reset(&qs);
	array_reset(&ps);
	array_reset(&co);
	return 0;
}
