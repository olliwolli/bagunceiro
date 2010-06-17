#ifndef _Z_FEATURES_H
#define _Z_FEATURES_H

#include <buffer.h>
#include <fmt.h>
#include <errno.h>

#define POSTDATA_MAX		1024 * 64
#define QUERY_MAX		128
#define COOKIE_MAX		128

#define PROGRAM_NAME		"CBLOG"

enum notice { N_ERROR, N_NOTE, N_NONE, N_ACTION };

/* Adds error messages that are not for free.
 * Adds approx. 4k
 * TODO: this also removes some useful notices.
 * */
//#define WANT_ERROR_PRINT

/* This removes uneeded part from the timestamp
 * when printing and parsing it from web page i/o */
//#define WANT_REDUCE_TS

/* Admin mode includes functions to change the database.
 * Use this if you want a cgi that does not allow write
 * access to the database and is smaller. This cgi
 * presumes that you save access to it by some kind of
 * external authentication method (e.g. http basic auth)
 * Adds approx. 8k */

//#define ADMIN_MODE

/* Admin mode pass allows password authentication
 * This mode only works over https. Since cookies may be
 * sniffed otherwise. It provides that action ?log.
 * For login with a password.
 * Note cookies may still be sniffed when browsing the page
 * in http mode while you still have a cookie.
 * */
//#define ADMIN_MODE_PASS

/* The database files are named using the same Timestamp
 * format as the blog output uses. Not so nice for files, but
 * saves a function */

//#define DEBUG
//#define DEBUG_PARSING
//#define DEBUG_PARSE_QUERY
//#define DEBUG_PARSE_POST
//#define DEBUG_ENTRY

#define sprint(str) buffer_puts(buffer_1, (str))
#define sprintm(...) buffer_putm(buffer_1, ##__VA_ARGS__)
#define sprintf(str) buffer_putsflush(buffer_1, (str))
#define sprintmf(...) buffer_putmflush(buffer_1, ##__VA_ARGS__)
#define sprintn(str, len) buffer_put(buffer_1, (str), (len))

/* YOU SHOULDN'T EDIT BELOW THIS LINE */
#ifdef NO_ADMIN_MODE
#undef ADMIN_MODE
#endif
#ifdef ADMIN_MODE_PASS
#include <openssl/sha.h>
#endif

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
struct errors {
	char note[128];
	int error;
	enum notice type;
} gerr;

inline void set_err(char *msg, int num, enum notice type)
{
	str_copy(gerr.note, msg);
	gerr.error = num;
	gerr.type = type;
}
#else
#define set_err(a,b,c)
#endif

#endif
