#ifndef _Z_CONF_H
#define _Z_CONF_H

#include <sys/types.h>

#include "z_blog.h"

int auth_conf(blog_t * conf, unsigned char *in, size_t len);
int add_session_id(char *buf);
int expire_all_sessions(blog_t * conf);
int validate_session_id(char *buf);

int load_config(blog_t * conf);
int save_config(blog_t * conf);

time_t http_if_modified_since(char *m);
#endif
