#ifndef _Z_BLOG_H
#define _Z_BLOG_H

#include <taia.h>
#include <sys/time.h>
#include <array.h>
#include "z_features.h"

#define CLOG_TYPE_RSS_2_0 1
#define CLOG_TYPE_HTML 2

#define FILE_BUFFER_SIZE 100

typedef struct query {
	enum qtype { QRY_TS, QRY_WEEK } type;
	enum qaction { QA_SHOW, QA_DELETE, QA_ADD, QA_MODIFY } action;
	unsigned int start;	/* offset from today, positive */
	unsigned int end;	/* offset from today, positive */
	array ts;
} query_t;

typedef struct blog {
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
	array css;
	enum csstype { CSS_DEFAULT, CSS_ERROR, CSS_RESET, CSS_SELECT,
		CSS_COOKIE
	} csstype;

	enum stype { S_HTML, S_RSS } stype;
	array input;
	struct timeval ptime;
	struct fmting *fmt;
} blog_t;

int load_config(blog_t * conf);

int print_blog(blog_t * conf);

int cdb_exist();
#endif
