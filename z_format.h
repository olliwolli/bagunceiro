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

typedef struct fmting {
	/* default formating */
	void (*day_entries) (const blog_t * conf, struct day_entry * e,
		size_t elen);
	void (*header) (const blog_t * conf);
	void (*footer) (const blog_t * conf);
} fmting_t;

void print_show(array * blog, blog_t * conf);
int print_add_entry(const blog_t * conf);
int print_mod_entry(const blog_t * conf, struct nentry *n);

extern struct fmting fmt_html;
extern struct fmting fmt_rss;
