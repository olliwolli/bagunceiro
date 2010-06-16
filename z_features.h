#ifndef _Z_FEATURES_H
#define _Z_FEATURES_H

#include <buffer.h>
#include <fmt.h>
#include <errno.h>

#define POSTDATA_MAX		1024 * 64
#define QUERY_MAX		128
#define COOKIE_MAX		64

#define PROGRAM_NAME		"CBLOG"

enum notice { N_ERROR, N_NOTE, N_NONE, N_ACTION };

struct errors {
	char note[128];
	int error;
	enum notice type;
} gerr;
//#define MAX_FILE_LENGTH 128 // FIXME
#ifdef NO_ADMIN_MODE
#undef ADMIN_MODE
#endif

//#define USE_COHERENT_TIME
//
//#define ADMIN_MODE
//#define DEBUG
//#define DEBUG_PARSING
//#define DEBUG_PARSE_QUERY
//#define DEBUG_PARSE_POST
//#define DEBUG_TIME
//#define DEBUG_ENTRY

#define sprint(str) buffer_puts(buffer_1, (str))
#define eprint(str) buffer_puts(buffer_2, (str))
#define sprintm(...) buffer_putm(buffer_1, ##__VA_ARGS__)
#define eprintm(...) buffer_putm(buffer_2, ##__VA_ARGS__)

#define sprintf(str) buffer_putsflush(buffer_1, (str))
#define eprintf(str) buffer_putsflush(buffer_2, (str))
#define sprintmf(...) buffer_putmflush(buffer_1, ##__VA_ARGS__)
#define eprintmf(...) buffer_putmflush(buffer_2, ##__VA_ARGS__)

#define sprintn(str, len) buffer_put(buffer_1, (str), (len))
#endif
