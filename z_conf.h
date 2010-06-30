#include "z_blog.h"
/* configuration */

#define CONF_FILE	"bconf.db"
int auth_conf(blog_t * conf, unsigned char *in, size_t len);
void add_session_id(char *buf);
int validate_session_id(char *buf);

int load_config(blog_t * conf);
