#ifndef _Z_BLOG_H
#define _Z_BLOG_H

#include <sys/time.h>

#include <taia.h>
#include <array.h>
#include <caltime.h>

#include "z_features.h"
#include "z_time.h"

#define SIZE_HTTP_ARG 64
#define SIZE_TAGLINE  SIZE_HTTP_ARG*3
#define SIZE_FIND_STR 24
#define URL_PATH 256

/* the default css stylesheet */
#define DEFAULT_STYLESHEET "style.css"

/* location of the database */
#define DB_FILE "db/db.inc"

/* qactions bigger than 1 need administration privileges */
#define IS_ADMIN_ACTION(x) (x) > 1

/* query */
#define QUERY_QRY "qry"
#define QUERY_TAG "tag"
#define QUERY_TS "ts"
#define QUERY_DEL "del"
#define QUERY_MOD "mod"
#define QUERY_ADD "add"
#define QUERY_CONFIG "config"
#define QUERY_CSS "css"
#define QUERY_FMT "fmt"
#define QUERY_MONTH "mn"
#define QUERY_LOGIN "login"
#define QUERY_LOGOUT "logout"

/* predefined database keys */
#define CONF_TITLE "title"
#define CONF_TAGLINE "tagline"
#define CONF_SEARCHBOX "searchbox"
#define CONF_PASS "input"

#define POST_TITLE "title"
#define POST_TAGLINE "tagline"
#define POST_PASS "pass"
#define POST_SEARCHBOX "sbox"
#define POST_INPUT "input"
#define POST_KEY "key"
#define POST_LOGIN "login"
#define POST_FILE_UPLOAD "file"
#define POST_ADD "add"
#define POST_MOD "mod"
#define POST_CONFIG "config"
#define POST_ACTION "action"

#define COOKIE_SID "sid"
#define COOKIE_CSS "css"

#define DB_LAST_MODIFIED "!lastmodified"

typedef struct query {
	/* query actions */
	enum qaction { QA_SHOW,
#ifdef ADMIN_MODE_PASS
		QA_LOGIN, QA_LOGOUT ,
#endif
#ifdef ADMIN_MODE
			QA_DELETE,
			QA_ADD, QA_ADD_POST,
			QA_MODIFY, QA_MODIFY_POST,
			QA_CONFIG, QA_CONFIG_POST
#endif
	} action;

	/* in case of QA_SHOW, distinct between different query types */
	enum qtype { QRY_TS, QRY_WEEK, QRY_MONTH, QRY_NONE, QRY_FIND, QRY_TAG } type;

	/* query timestamp (?ts=) */
	char ts[FMT_TAIA_HEX];

	/* query days (e.g. for the last 7 days, start=0, doff=8 )*/
	unsigned int start;	/* offset from today, positive */
	unsigned int doff;	/* offset from start, positive */

	/* query month (?mon=) */
	struct caltime mon;

#ifdef WANT_SEARCHING
	/* query search term, case sensitive (?qry=) */
	char find[SIZE_FIND_STR];
#endif

#ifdef WANT_TAGS
	/* tag search term, case insensitive (?tag=) */
	char tag[SIZE_FIND_STR];
#endif

	/* posted blog entry */
	array input;

	/* configuration parameters */
#if defined(ADMIN_MODE) && defined(WANT_CGI_CONFIG)
	char pass[SIZE_HTTP_ARG];
	char title[SIZE_HTTP_ARG];
	char tagline[SIZE_TAGLINE];
#endif

	/* formatting (?fmt=) */
	enum stype { S_HTML, S_RSS } stype;

	/* css stylesheet (?css=) */
	char css[SIZE_HTTP_ARG];
	enum csstype { CSS_DEFAULT, CSS_ERROR, CSS_RESET, CSS_SELECT,
		CSS_COOKIE
	} csstype;


} query_t;

typedef struct blog {

/* authentication */
#ifdef ADMIN_MODE_PASS
	int auth;
	enum authtype { AUTH_NONE, AUTH_POST, AUTH_SID } authtype;
	/* session id */
	char sid[SIZE_SESSION_ID+1];
#endif

/* basic info */
	char title[SIZE_HTTP_ARG];
	char tagline[SIZE_TAGLINE];
	char db[SIZE_HTTP_ARG];

#ifdef WANT_SEARCHING
	/* inline search box */
	char sbox;
#endif

/* parameters */
	query_t qry;

/* environment variables */
	char *ssl;

#define MAX_SCRIPT_SIZE 128
	char script[MAX_SCRIPT_SIZE];
#define MAX_HOST_SIZE 128
	char host[MAX_HOST_SIZE];
	char *mod;

	char path[URL_PATH];

/* parsed info */
	struct timeval ptime;
	struct taia now;
	struct fmting *fmt;

} blog_t;

int handle_query(blog_t * conf);

#endif
