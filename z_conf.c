#include <array.h>
#include <byte.h>
#include <str.h>

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

	byte_zero(&value, sizeof(array));
	byte_zero(&file, sizeof(array));

	choose_file(&file, conf->db);

	err = cdb_get(file.p, key, ks, &value);
	if (err < 0 )
		return err;

	byte_copy(dest, array_bytes(&value), value.p);
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

	auth = byte_equal(pbuf, SHA256_DIGEST_LENGTH, in);
	byte_zero(pbuf, SHA256_DIGEST_LENGTH);

	return auth;
}

//void auth_init(blog_t * conf, unsigned char *in, size_t len)
//{
//	unsigned char tbuf[SHA256_DIGEST_LENGTH];
//	array value, file;
//
//	byte_zero(&value, sizeof(array));
//	byte_zero(&file, sizeof(array));
//
//	SHA256((unsigned char *)"passwordstring", 10, tbuf);
//	array_catb(&value, (char *)tbuf, SHA256_DIGEST_LENGTH);
//	cdb_add("db/bconf.db", "input", 5, &value);
//
//	array_reset(&file);
//	array_reset(&value);
//}

#endif

int load_config(blog_t * conf)
{
	int err;

	err = read_conf(conf, "title", 5, conf->title);

	return err;
}
