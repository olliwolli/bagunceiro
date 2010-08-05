#ifndef _Z_FEATURES_H
#define _Z_FEATURES_H

#include <buffer.h>
#include <fmt.h>
#include <str.h>
#include <errno.h>

#define POSTDATA_MAX		1024 * 64
#define QUERY_MAX		128
#define COOKIE_MAX		128

#define PROGRAM_NAME		"b"

/* Adds error messages that are not for free. Adds approx. 4k.
 * This also removes some useful notices. Ok for admin binary */
//#define WANT_ERROR_PRINT

/* This removes parts from the packed timestamp
 * when printing and parsing it from web page i/o */
#define WANT_REDUCE_TS

/* date is printed in erisian calendar
 * https://secure.wikimedia.org/wikipedia/en/wiki/Discordian_calendar
 * TODO: season browsing not implemented */
//#define WANT_ERISIAN_CALENDAR

/* parameters for WANT_REDUCE_TS:
 * REDUCE_SIZE 16 will break on 2106-02-07 by overflowing after
 * a.k.a. unsigned Y2K38 problem
 * this reduction:
 * 40000000ffffffffffffffff00000000 -> ffffffffffffffff
 * If you want to be safe for the future, increase REDUCE_SIZE
 * MAX is 24
 * The last four bytes will always be removed (00000000)
 * if you activate WANT_REDUCE_TS */
#ifdef WANT_REDUCE_TS
#define REDUCE_SIZE 16
#define REDUCE_OFFSET (24 - REDUCE_SIZE)
#endif

/* Define this macro if you want to use the tinymce html editor
 * if you do not want this you may also safely delete the
 * installed tinymce directory */
#define WANT_TINY_HTML_EDITOR
/* TINY_HTML_PATH the path to the editor */
//#define TINY_HTML_PATH "/tinyeditor/"

/* enable month browsing of blog */
#define WANT_MONTH_BROWSING

/* upload form on post page, if you disable this you also might
 * want to delete the two files mentioned below */
#define WANT_UPLOAD
#define UPLOAD_JS "/upload.js"
#define UPLOAD_CGI "http://localhost/cgi-bin/clog/upload.cgi"

/* full text searching capability */
#define WANT_SEARCHING

/* case insensitive full text searching capability */
#define WANT_TAGS

/* want http "304 Not Modified" support, you usually want this */
#define WANT_HTTP_304

/* Debugging options */
//#define DEBUG
//#define DEBUG_PARSING
//#define DEBUG_ENTRY
/* loops through default action x100 */
//#define DEBUG_MEMORY 100

/* YOU SHOULDN'T EDIT BELOW THIS LINE */
/* NOTE:
 * usually the makefile will handle these first three symbols,
 * blog.cgi does not define the following 2 symbols but
 * NO_ADMIN_MODE. And admin.cgi defines the following two symbols
 * WANT_FAST_CGI is also handled by makefile */

/* Admin mode includes functions to change the database. Use
 * this if you want an executable that does not allow write
 * access to the database and is smaller. If you don't define
 * ADMIN_MODE_PASS, the resulting executable will assume that
 * you secure access to it by some kind of external
 * authentication method (e.g. http basic auth). Adds approx.
 *  8k */
//#define ADMIN_MODE

/* Admin mode pass allows password authentication. The mode only
 * works over https. Since cookies and passwords may be sniffed
 * otherwise. It provides most notably the actions ?login, ?add,
 * ?mod, ?del, ?logout, ?config. */
//#define ADMIN_MODE_PASS

/*	- free all malloced memory at each loop runtime
 * 	- reset variables at beginning of loop
 * Define in order to use fastcgi instead of cgi
 * This requires changes to your webserver setup */
//#define WANT_FAST_CGI
#ifdef NO_ADMIN_MODE
#undef ADMIN_MODE
#endif

#ifdef ADMIN_MODE_PASS
/* needed for login error messages */
#define WANT_ERROR_PRINT
#endif

#ifdef ADMIN_MODE
#define WANT_CGI_CONFIG
#endif

#ifdef WANT_FAST_CGI
#include "fcgi_config.h"
#include "fcgiapp.h"

FCGX_Stream *fcgi_in, *fcgi_out, *fcgi_err;
FCGX_ParamArray envp;

#define getenv(str) FCGX_GetParam((str), envp)
#define FCGX_PutSm(b,...) FCGX_PutSm_internal(b,__VA_ARGS__,(char*)0)

#define sprint(str) FCGX_PutS(str, fcgi_out)
#define sprintm(...) FCGX_PutSm(fcgi_out, ##__VA_ARGS__)
#define sprintf(str) FCGX_PutS(str, fcgi_out)
#define sprintmf(...) FCGX_PutSm(fcgi_out, ##__VA_ARGS__)
#define sprintn(str, len) FCGX_PutStr(str, len, fcgi_out)

#ifdef WANT_ERROR_PRINT
#define eprint(str) FCGX_PutS(str, fcgi_err)
#define eprintm(...) FCGX_PutSm(fcgi_err, ##__VA_ARGS__)
#define eprintf(str) do {FCGX_PutS(str, fcgi_err);} while(0)
#define eprintmf(...) do{FCGX_PutSm(fcgi_err);} while(0)
#define eprintn(str, len) FCGX_PutStr(str, len, fcgi_err)
#endif
#else
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
#endif
#endif

#ifndef WANT_ERROR_PRINT
#define eprint(str)
#define eprintm(...)
#define eprintf(str)
#define eprintmf(...)
#endif

#ifdef WANT_ERROR_PRINT
#include <string.h>
enum notice { N_ERROR, N_NOTE, N_NONE, N_ACTION };
struct errors {
	char note[128];
	int error;
	enum notice type;
} gerr;

#if defined(DEBUG_ENTRY) ||\
	defined(DEBUG_PARSING)
#define DEBUG
#endif

/* activate and set for input simulation */
#ifdef DEBUG
extern char debug_query[];
extern char debug_post[];
extern char debug_cookie[];
#endif

static inline void set_err(char *msg, int num, enum notice type)
{
	strcpy(gerr.note, msg);
	gerr.error = num;
	gerr.type = type;
}
#else
#define set_err(a,b,c)
#endif

#define SIZE_SESSION_ID 32
/* the following two macros should represent
 * the same value, spares some conversion */
/* session validity in seconds */
#define SESSION_VTIME 43200
/* session validity in seconds as string */
#define SESSION_STR_VTIME "43200"

#endif
