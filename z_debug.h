#ifndef _Z_DEBUG_H
#define _Z_DEBUG_H

#include <array.h>
#include "z_features.h"
#include "z_blog.h"

extern array debugmsg;
extern char daction[4][10];

void __d(const char *desc, const char *value);
void debug_print(const blog_t * conf, array * ps, array * qs, array * co);

#endif
