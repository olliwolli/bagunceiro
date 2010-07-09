#ifndef _Z_BLOG_H
#define _Z_BLOG_H

#include <taia.h>
#include <sys/time.h>
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
	enum qtype { QRY_TS, QRY_WEEK, QRY_MONTH, QRY_NONE, QRY_FIND } type;

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
	char *script;
	char *host;
	char *mod;

	char path[URL_PATH];

/* parsed info */
	struct timeval ptime;
	struct taia now;
	struct fmting *fmt;

} blog_t;

int handle_query(blog_t * conf);

#endif
