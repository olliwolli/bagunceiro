#ifndef _Z_FORMAT_H
#define _Z_FORMAT_H
/*
 *  format.h
 *
 *  Oliver-Tobias Ripka (otr), otr@bockcay.de, 14.06.2010,
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#include "z_entry.h"
#include "z_day.h"
#include "z_result.h"

typedef struct fmting {
	void (*day_entries) (const blog_t * conf, struct day * e,
		size_t elen);
	int (*header) (const blog_t * conf);
	void (*footer) (const blog_t * conf);
	void (*noentries) (const blog_t * conf);
} fmting_t;

int print_show(struct result * blog, blog_t * conf);
#ifdef ADMIN_MODE
int print_add_entry(const blog_t * conf);
int print_mod_entry(const blog_t * conf, struct nentry *n);
int print_config(const blog_t * conf);
#endif

#ifdef ADMIN_MODE_PASS
void print_login(const blog_t * conf);
#endif

void fmt_key_plain(struct nentry *e, char * fmt);
#define FMT_PERMA_STATIC  (1 + FMT_TAIA_HEX + 4 + PROTO_HTTP_LEN)
int fmt_perma_link(const blog_t * conf, struct nentry *e, char * fmt);

extern struct fmting fmt__html;
extern struct fmting fmt__rss;
#endif
