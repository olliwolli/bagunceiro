#include "z_blog.h"
/* configuration */

#define CONF_FILE	"bconf.db"
int auth_conf(blog_t * conf, unsigned char *in, size_t len);

int load_config(blog_t * conf);
