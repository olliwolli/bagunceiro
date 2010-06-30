#ifndef _Z_FEATURES_H
#define _Z_FEATURES_H

#include <buffer.h>
#include <fmt.h>
#include <errno.h>

#define POSTDATA_MAX		1024 * 64
#define QUERY_MAX		128
#define COOKIE_MAX		128

#define PROGRAM_NAME		"BACALHAU"

/* Adds error messages that are not for free.
 * Adds approx. 4k
 * this also removes some useful notices.
 * */
//#define WANT_ERROR_PRINT

/* This removes parts from the packed timestamp
 * when printing and parsing it from web page i/o */
#define WANT_REDUCE_TS

/* parameters for WANT_REDUCE_TS:
 * REDUCE_SIZE 16 will break on 2106-02-07 by overflowing after
 * this reduction:
 * 40000000ffffffffffffffff00000000 -> ffffffffffffffff
 * If you want to be safe for the future, increase REDUCE_SIZE
 * MAX is 24
 * The last four bytes will always be removed (00000000)
 * if you activate WANT_REDUCE_TS
 */
#ifdef WANT_REDUCE_TS
#define REDUCE_SIZE 16
#define REDUCE_OFFSET (24 - REDUCE_SIZE)
#endif

#ifdef ADMIN_MODE
#define WANT_CGI_CONFIG
#endif
/* Admin mode includes functions to change the database.
 * Use this if you want a cgi that does not allow write
 * access to the database and is smaller. This cgi
 * presumes that you save access to it by some kind of
 * external authentication method (e.g. http basic auth)
 * Adds approx. 8k */
//#define ADMIN_MODE

/* Admin mode pass allows password authentication
 * This mode only works over https. Since cookies may be
 * sniffed otherwise. It provides the action ?login.
 * */
//#define ADMIN_MODE_PASS

/* Define this macro if you want to use the tiny html editor;
 * It has to be downloaded seperately from:
 * http://www.leigeber.com/2010/02/javascript-wysiwyg-editor/
 * We use packed.js instead of tinyeditor.js
 * TINY_HTML_PATH is the base url to the editor in your webserver
 * setup */
#define WANT_TINY_HTML_EDITOR
#define TINY_HTML_PATH "/tinyeditor/"


#define WANT_UPLOAD
#define UPLOAD_JS "/upload.js"
#define UPLOAD_CGI "/upload.pl"
/* define in order to be able to browse months on the blog */
#define WANT_MONTH_BROWSING

/* TODO:unimplemented:
 * 	- free all malloced memory at each loop runtime
 * 	- exclude printf like function from fast cgi library
 * 	- reset variables at beginning of loop
 * 	- add linker flag to makefile
 * define in order to use fast cgi */
//#define WANT_FAST_CGI

/* Debugging options */
//#define DEBUG
//#define DEBUG_PARSING
//#define DEBUG_ENTRY

/* activate and set for input simulation */
//#define DEBUG_PARSE_QUERY "config"
//#define DEBUG_PARSE_POST "action=config&title=Your+new+blog2&tagline=Your+tagline&input="
//#define DEBUG_PARSE_COOKIE "sid=0a9b12e19184457041466032126e4655"

//#define ADMIN_OVERRIDE
#define SESSION_ID_LEN 32
/*  validity in seconds, FIXME unify */
#define SESSION_VTIME 43200
#define SESSION_STR_VTIME "43200"

#define DEFAULT_STYLESHEET "style.css"
#define DB_FILE "db/db.inc"

/* YOU SHOULDN'T EDIT BELOW THIS LINE */
#ifdef WANT_FAST_CGI
#include "fcgi_stdio.h"
#endif

#ifdef NO_ADMIN_MODE
#undef ADMIN_MODE
#endif
#ifdef ADMIN_MODE_PASS
#include <openssl/sha.h>
#endif

#define sprint(str) buffer_puts(buffer_1, (str))
#define sprintm(...) buffer_putm(buffer_1, ##__VA_ARGS__)
#define sprintf(str) buffer_putsflush(buffer_1, (str))
#define sprintmf(...) buffer_putmflush(buffer_1, ##__VA_ARGS__)
#define sprintn(str, len) buffer_put(buffer_1, (str), (len))

#ifdef WANT_ERROR_PRINT
#define eprint(str) buffer_puts(buffer_2, (str))
#define eprintm(...) buffer_putm(buffer_2, ##__VA_ARGS__)
#define eprintf(str) buffer_putsflush(buffer_2, (str))
#define eprintmf(...) buffer_putmflush(buffer_2, ##__VA_ARGS__)
#else
#define eprint(str)
#define eprintm(...)
#define eprintf(str)
#define eprintmf(...)
#endif

#ifdef WANT_ERROR_PRINT
enum notice { N_ERROR, N_NOTE, N_NONE, N_ACTION };
struct errors {
	char note[128];
	int error;
	enum notice type;
} gerr;

static inline void set_err(char *msg, int num, enum notice type)
{
	str_copy(gerr.note, msg);
	gerr.error = num;
	gerr.type = type;
}
#else
#define set_err(a,b,c)
#endif

#endif
