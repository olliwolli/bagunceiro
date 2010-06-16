#include <array.h>
#include <byte.h>
#include <str.h>
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
	if (err)
		return err;

	str_copy(dest, value.p);
	array_reset(&file);
	array_reset(&value);

	return 0;
}

int load_config(blog_t * conf)
{
	int err;

	err = read_conf(conf, "title", 5, conf->title);

	return err;
}
