#ifndef _Z_BLOG_H
#define _Z_BLOG_H

#include <taia.h>
#include <sys/time.h>
#include <array.h>
#include "z_features.h"
#include "z_time.h"

#define CLOG_TYPE_RSS_2_0 1
#define CLOG_TYPE_HTML 2

#define FILE_BUFFER_SIZE 100

typedef struct query {
	enum qtype { QRY_TS, QRY_WEEK } type;
	enum qaction { QA_SHOW,
#ifdef ADMIN_MODE
			QA_DELETE, QA_ADD, QA_MODIFY,
#endif
#ifdef ADMIN_MODE_PASS
		QA_LOGIN,
#endif
	} action;
	unsigned int start;	/* offset from today, positive */
	unsigned int end;	/* offset from today, positive */
	char ts[FMT_TAIA_HEX];
} query_t;

typedef struct blog {
#ifdef ADMIN_MODE_PASS
	int authcache;
	int authpost;
	unsigned char token[SHA256_DIGEST_LENGTH];
	int ssl;
#endif
/* basic info */
	char title[20];
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
	struct fmting *fmt;
} blog_t;

int handle_query(blog_t * conf);

#endif
