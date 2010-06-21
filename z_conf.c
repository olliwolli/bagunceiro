#include <array.h>
#include <str.h>

#include <openssl/sha.h>
#include "z_features.h"

#include "z_cdb.h"
#include "z_conf.h"

static void choose_file(array * file, const char *dbpath)
{
	array_cats(file, dbpath);
	array_cats0(file, CONF_FILE);
}

static int read_conf(blog_t * conf, char *key, size_t ks, char *dest)
{
	int err;
	array value, file;

	memset(&value, 0, sizeof(array));
	memset(&file, 0, sizeof(array));

	choose_file(&file, conf->db);

	err = cdb_get(file.p, key, ks, &value);
	if (err < 0 )
		return err;

	memcpy(dest, value.p, array_bytes(&value));
	array_reset(&file);
	array_reset(&value);

	return 0;
}

#ifdef ADMIN_MODE_PASS
int auth_conf(blog_t * conf, unsigned char *in, size_t len)
{
	unsigned char pbuf[SHA256_DIGEST_LENGTH];
	int auth;

	read_conf(conf, "input", 5, (char *)pbuf);

	auth = !memcmp(pbuf, in, SHA256_DIGEST_LENGTH);
	memset(pbuf, 0, SHA256_DIGEST_LENGTH);

	return auth;
}

#endif

int load_config(blog_t * conf)
{
	int err;

	err = read_conf(conf, "title", 5, conf->title);

	return err;
}
