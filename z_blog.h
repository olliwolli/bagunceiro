#ifndef _Z_BLOG_H
#define _Z_BLOG_H

#include <taia.h>
#include <sys/time.h>
#include <array.h>
#include <caltime.h>
#include "z_features.h"
#include "z_time.h"

#define MAX_CONF_STR 256
typedef struct query {
	enum qtype { QRY_TS, QRY_WEEK, QRY_MONTH, QRY_NONE } type;

	enum qaction { QA_SHOW,
#ifdef ADMIN_MODE
			QA_DELETE, QA_ADD, QA_MODIFY, QA_CONFIG,
#endif
#ifdef ADMIN_MODE_PASS
		QA_LOGIN, QA_LOGOUT 
#endif
	} action;

	unsigned int start;	/* offset from today, positive */
	unsigned int doff;	/* offset from start, positive */
	char ts[FMT_TAIA_HEX]; /* +1 for a zero */

/*  configuration */
#if defined(ADMIN_MODE) && defined(WANT_CGI_CONFIG)
	char pass[MAX_CONF_STR];
	char title[MAX_CONF_STR];
	char tagline[MAX_CONF_STR];
#endif

	struct caltime mon;
} query_t;

typedef struct blog {

#ifdef ADMIN_MODE_PASS
	int auth;
	enum authtype { AUTH_NONE, AUTH_POST, AUTH_SID } authtype;
	char sessionid[SESSION_ID_LEN+1];
	enum ssl {SSL_OFF, SSL_ON} ssl;
#endif


/* basic info */
	char title[20];
	char tagline[256];
	char db[128];

/* parameters */
	query_t qry;

/* environment variables */
	char *script;
	char *host;
	char *cookie;

/* parsed info */
#define MAX_CSS_ARG	10
	char css[MAX_CSS_ARG];
	enum csstype { CSS_DEFAULT, CSS_ERROR, CSS_RESET, CSS_SELECT,
		CSS_COOKIE
	} csstype;

	enum stype { S_HTML, S_RSS } stype;
	array input;
	struct timeval ptime;
	struct taia now;
	struct fmting *fmt;

} blog_t;

int handle_query(blog_t * conf);

#endif
