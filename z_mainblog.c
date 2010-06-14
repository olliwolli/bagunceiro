#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <byte.h>
#include <textcode.h>

#include "z_blog.h"
#include "z_time.h"
#include "z_features.h"

static array debugmsg;

/* DEBUGGING */
#ifdef DEBUG_PARSING
static char daction[4][10] = { "Show", "Delete", "Add", "Modify" };
static void __d(const char *desc, const char *value)
{
	if (value) {
		array_cats(&debugmsg, desc);
		array_cats(&debugmsg, value);
		array_cats(&debugmsg, "<br>\n");
	} else {
		array_cats(&debugmsg, desc);
		array_cats(&debugmsg, "[Empty]<br>\n");
	}
}

static void debug_print(const blog_t * conf)
{
	__d("Cookie is: ", conf->cookie);
	__d("URL is: ", conf->script);
	__d("Host is: ", conf->host);
	__d("CSS arg is: ", conf->css.p);

	if (array_bytes(&conf->input))
		__d("Input is: ", conf->input.p);
	if (array_bytes(&conf->qry.ts))
		__d("Key is: ", conf->qry.ts.p);

	__d("Action is: ", daction[conf->qry.action]);
}
#else
static void debug_print(const blog_t * conf)
{
}

static void __d(const char *desc, const char *value)
{
}
#endif

#ifdef DEBUG_PARSE_POST
static void get_post_string(array * ps)
{
	byte_zero(postdata, sizeof(array));
	array_cats0(ps, "action=mod&key=400000004c15216810ace"
		"1fc00000000&input=29304324%0D%0Asdfsdf%0D%0Asfddsf");
	__d("Post string is: ", ps->p);
}
#endif
#ifdef DEBUG_PARSE_QUERY
static void get_query_string(array * qs)
{
	byte_zero(qs, sizeof(array));
	array_cats0(qs, "add");
//      array_cats0(qs, "mod=400000004c15216810ace1fc00000000&css=style.css");
	__d("Query string is: ", qs->p);
}
#endif

/* CONVERSION */
/* TODO: avoid temporary uncoded */
void unencode(char *coded, array * arruncoded)
{

	int len = strlen(coded);
	size_t destlen;

	byte_zero(arruncoded, sizeof(array));
	char buf[len * 3];
	scan_urlencoded(coded, buf, &destlen);
	array_catb(arruncoded, buf, destlen);
	array_cat0(arruncoded);
}

/* PARSING  */
static int get_param(char *q_str, char *search, array * ret, size_t n)
{
	char *sep = "&";
	char *sep2 = "=";
	char *query, *tokptr;
	char *key, *val, *tokptr2;

	/* not found */
	if (strstr(q_str, search) == NULL)
		return -1;

	for (query = strtok_r(q_str, sep, &tokptr);
		query != NULL; query = strtok_r(NULL, sep, &tokptr)) {

		key = strtok_r(query, sep2, &tokptr2);
		val = strtok_r(NULL, sep2, &tokptr2);

		if (strcmp(key, search) == 0) {
			/* just a key, no value */
			if (val == 0)
				return -2;

			/* not complying to length rules */
			if (strlen(val) >= n && n != -1) {
				array_cats0(ret, "bad");
				return -3;
			}
			unencode(val, ret);
			return 0;	/* alright */
		}
	}
	return 1;
}

static void parse_cookie(blog_t * conf, array * co)
{
	char gets[COOKIE_MAX] = "";
	int err;

	if (array_bytes(co)) {
		strncpy(gets, co->p, COOKIE_MAX);
		gets[COOKIE_MAX - 1] = '\0';

		err = get_param(gets, "css", &conf->css, COOKIE_MAX);
		if (!err) {
			conf->csstype = CSS_COOKIE;
		}
	}
}

static void parse_query(blog_t * conf, array * qs)
{
	char gets[QUERY_MAX];
	int err;

	if (array_bytes(qs)) {

		strcpy(gets, qs->p);
		err = get_param(gets, "ts", &conf->qry.ts, 50);
		if (!err) {
			conf->qry.type = QRY_TS;
			conf->qry.action = QA_SHOW;
		}

		strcpy(gets, qs->p);
		array tmpcss;
		byte_zero(&tmpcss, sizeof(tmpcss));
		err = get_param(gets, "css", &tmpcss, 20);

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
			array_cats0(&conf->css, tmpcss.p);
			break;
		}
		array_reset(&tmpcss);

		array type; // TODO redefine
		byte_zero(&type, sizeof(array));
		strcpy(gets, getenv("QUERY_STRING"));
		err = get_param(gets, "fmt", &type, 20);
		if(err == 0 && !strcmp(type.p, "rss")){
			conf->stype = S_RSS;
		}
		array_reset(&type);

#ifdef ADMIN_MODE
		strcpy(gets, qs->p);
		err = get_param(gets, "del", &conf->qry.ts, 50);
		if (!err) {
			conf->qry.type = QRY_TS;
			conf->qry.action = QA_DELETE;
		}

		strcpy(gets, qs->p);
		err = get_param(gets, "add", NULL, 50);
		if (err == -2)
			conf->qry.action = QA_ADD;

		strcpy(gets, qs->p);
		err = get_param(gets, "mod", &conf->qry.ts, 50);
		if (!err) {
			conf->qry.type = QRY_TS;
			conf->qry.action = QA_MODIFY;
		}
#endif
	}
}

#ifdef ADMIN_MODE
static void parse_postdata(blog_t * conf, array * pd)
{
	char *gets;
	array tmp;
	byte_zero(&tmp, sizeof(array));

	if (array_bytes(pd)) {

		gets = malloc(array_bytes(pd));
		strcpy(gets, pd->p);
		get_param(gets, "input", &conf->input, -1);

		strcpy(gets, pd->p);
		get_param(gets, "key", &conf->qry.ts, 50);

		strcpy(gets, pd->p);
		if (get_param(gets, "action", &tmp, 4) == 0) {
			if (!strcmp(tmp.p, "add"))
				conf->qry.action = QA_ADD;
			if (!strcmp(tmp.p, "mod"))
				conf->qry.action = QA_MODIFY;
		}
		free(gets);
	}
	array_reset(&tmp);
}
#else
static void parse_postdata(blog_t * conf)
{
}
#endif

#ifndef DEBUG_PARSE_QUERY
static void get_query_string(array * qs)
{
	byte_zero(qs, sizeof(array));
	if (getenv("QUERY_STRING") != NULL)
		array_cats0(qs, getenv("QUERY_STRING"));

	__d("Query string is: ", qs->p);
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
		__d("Post string is: ", ps->p);
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
				.type = QRY_WEEK,
				.start = 0,
			.end = 8},
		.stype = S_HTML,
		.cookie = "",
		.csstype = CSS_DEFAULT,
	};
	byte_zero(&conf.input, sizeof(array));
	byte_zero(&conf.css, sizeof(array));

	time_start(&conf.ptime);

	load_config(&conf);

	conf.host = getenv("SERVER_NAME");
	conf.script = getenv("SCRIPT_NAME");

	/* get the strings */
	get_cookie_string(&co);
	get_post_string(&ps);
	get_query_string(&qs);

	/* parse cookie first */
	parse_cookie(&conf, &co);
	parse_postdata(&conf, &ps);
	parse_query(&conf, &qs);

	/* set a cookie */
	if (conf.csstype != CSS_DEFAULT && conf.csstype != CSS_RESET) {
		if (access(conf.css.p, F_OK) == -1)
			conf.csstype = CSS_ERROR;
	}

	print_blog(&conf);

#ifdef DEBUG_PARSING
	debug_print(&conf);
	array_cat0(&debugmsg);
	sprint("<div id=\"note\"><div id=\"head\">DEBUG</div>");
	sprintm(debugmsg.p, "</div>");
#endif

	time_stop_print(&conf.ptime);

	array_reset(&qs);
	array_reset(&ps);
	array_reset(&co);
	array_reset(&debugmsg);
	return 0;
}
