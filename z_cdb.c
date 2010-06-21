#include <unistd.h>
#include <stdio.h>
#include <dirent.h>

#include <cdb_make.h>
#include <str.h>
#include <cdb.h>
#include <open.h>
#include <alloca.h>
#include <array.h>
#include <errno.h>

#include "z_cdb.h"
#include "z_entry.h"
#include "z_features.h"

#ifdef ADMIN_MODE
typedef enum cop {
	OP_CPY, OP_MOD, OP_DEL, OP_ADD
} cop_t;

#define OP_CPY 0x00
#define OP_MOD 0x01
#define OP_DEL 0x02
#define OP_ADD 0x03
#define CP_NOP 0x00
#define CP_PAR 0x01
#define CP_SRC 0x02

inline static int exists(const char *file)
{
	if (access(file, F_OK | W_OK) != -1) {
		return 1;
	}
	return 0;
}

inline static size_t __make_tmp_name(char *tmpname, const char *file)
{
	str_copy(tmpname, file);
	strcat(tmpname, ".tmp");

	return 0;
}

int cdb_init_file(const char *file)
{
	int fp;
	struct cdb_make cdbm;

	if (exists(file)) {
		return 0;
	} else {
		fp = open_write(file);
		cdb_make_start(&cdbm, fp);
		cdb_make_finish(&cdbm);
		close(fp);
		return 1;
	}
}

/* dirty */
static int __cdb_remake_real(const char *name, const char *newname,
	const unsigned char *skey, const
	size_t slen, unsigned char *value, size_t vlen, enum cop op)
{
	struct cdb_make cdbm;
	int numrec = 0;
	int first, loop_copy_src, new, old;
	struct cdb c;
	uint32 kpos;
	uint32 kp, klen, dp, dlen;
	unsigned char *key, *data;
	int found = 0;

	new = open_write(newname);
	if (new == -1) {
		exists(newname);
	}
	old = open_read(name);
	if (old == -1) {
		exists(name);
		return -1;
	}

	cdb_init(&c, old);
	first = cdb_firstkey(&c, &kpos);
	cdb_make_start(&cdbm, new);

	if (OP_ADD == op) {
		cdb_make_add(&cdbm, skey, slen, value, vlen);
		++numrec;

		if (first == -1 || first == 0)
			goto finish;

	}

	if (!first)
		return 0;
	do {
		loop_copy_src = CP_SRC;

		kp = cdb_keypos(&c);
		klen = cdb_keylen(&c);

		key = alloca(klen);

		cdb_read(&c, key, klen, kp);

		/* if the key is the same modifiy the entry */
		switch (op) {
		case OP_MOD:
			if (!memcmp
				((const char *)key, (const char *)skey, klen)) {
				found = 1;
				loop_copy_src = CP_PAR;
			}

			break;
		case OP_DEL:
			if (!memcmp
				((const char *)key, (const char *)skey, klen)) {
				loop_copy_src = CP_NOP;
				found = 1;
			}
			break;
		case OP_CPY:
			/* fall-through */
		default:
			break;
		}

		switch (loop_copy_src) {
		case CP_PAR:
			data = value;
			dlen = str_len((const char *)value);
			cdb_make_add(&cdbm, key, klen, data, dlen);
			++numrec;
			break;
		case CP_NOP:
			break;
		case CP_SRC:
			/*  fall-through */
		default:
			++numrec;
			/* by default copy from src */
			dp = cdb_datapos(&c);
			dlen = cdb_datalen(&c);
			data = alloca(dlen);
			cdb_read(&c, data, dlen, dp);
			cdb_make_add(&cdbm, key, klen, data, dlen);
		}
	} while (cdb_nextkey(&c, &kpos) == 1);

finish:
	cdb_make_finish(&cdbm);
	close(c.fd);
	close(cdbm.fd);

	if (op == OP_DEL || op == OP_MOD)
		if (!found) {
			errno = ENOKEY;
			return -2;
		}

	return numrec;
}

static int __cdb_remake(const char *file, const char *k, const size_t ks,
	const array * v, enum cop op)
{
	//static array tmpfile;
	char tmpfile[str_len(file) + 1];
	int err;

	cdb_init_file(file);
	__make_tmp_name(tmpfile, file);

	if (v != NULL) {
		err = __cdb_remake_real(file, tmpfile,
			(unsigned char *)k, ks,
			(unsigned char *)v->p, array_bytes(v), op);
	} else {
		err = __cdb_remake_real(file, tmpfile,
			(unsigned char *)k, ks, NULL, 0, op);
	}

/*
 *	FIXME:
 * 	For reliability, dbmake first constructs the cdb database in a temporary
 * 	file.Then, after the temporary file is successfully written to disk,
 * 	it is "atomically" moved into the target output file location.
 *
 * 	So no temporary file on our part needed!
 */

	rename(tmpfile, file);

	if (err == 0) {
		unlink(file);
	}
	return err;
}

int cdb_mod(const char *file, const char *k, const size_t ks, const array * v)
{
	return __cdb_remake(file, k, ks, v, OP_MOD);
}

int cdb_add(const char *file, const char *k, size_t ks, const array * v)
{
	return __cdb_remake(file, k, ks, v, OP_ADD);
}

int cdb_del(const char *file, const char *k, const size_t ks)
{
	return __cdb_remake(file, k, ks, NULL, OP_DEL);
}

#endif

int cdb_get(const char *file, const char *k, const size_t ks, array * v)
{
	struct cdb result;
	int err, fd, len;
	int first;
	unsigned char buf[MAX_ENTRY_SIZE];

	first = 1;
	fd = open_read(file);
	if (fd <= 0) {
		return -1;
	}

	cdb_init(&result, fd);
	err = cdb_find(&result, (unsigned char *)k, ks);

	if (err == 0) {
		return 0;
	} else if (err == -1) {
		return -1;
	} else {
		do {

			len = cdb_datalen(&result);
			err = cdb_read(&result, buf, len, cdb_datapos(&result));
			if (err < 0) {
				return -2;
			}
			/* seperates records by ":" */
			if (!first)
				array_cats(v, ":");
			array_catb(v, (char *)buf, len);
			first = 0;

		} while (cdb_findnext(&result, (unsigned char *)k, ks) > 0);

		return 1;
	}
}

int cdb_read_all(const char *name, array * entries, struct eops *ops)
{
	int numdump = 0;
	int first;
	struct cdb c = { 0 };
	uint32 kpos;
	void *tmp;

	uint32 kp, klen, dp, dlen;
	unsigned char *key, *data;

	int fd = open_read(name);
	if (fd <= 0) {
		return -1;
	}
	cdb_init(&c, fd);
	first = cdb_firstkey(&c, &kpos);
	if (!first)
		return 0;
	do {
		kp = cdb_keypos(&c);
		klen = cdb_keylen(&c);
		dp = cdb_datapos(&c);
		dlen = cdb_datalen(&c);

		key = alloca(klen);
		data = alloca(dlen + 4);
		cdb_read(&c, key, klen, kp);
		cdb_read(&c, data, dlen, dp);

		/* key not found */
		if (__unlikely(klen == 0)) {
			return -2;
		}

		/* generic way */
		tmp = ops->alloc();
		ops->add_val(tmp, data, dlen);
		ops->add_key(tmp, key, klen);
		ops->add_to_array(tmp, entries);
		++numdump;
	} while (cdb_nextkey(&c, &kpos) == 1);

	close(fd);
	cdb_free(&c);

	return numdump;
}

int cdb_search(const char *dbpath, const char *needle, array * entries)
{
	DIR *d;
	struct dirent *dir;
	d = opendir(dbpath);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if (dir->d_name[0] == '.'
				|| !strcmp(dir->d_name, "bconf.db"))
				continue;

			printf("%s\n", dir->d_name);
		}

		closedir(d);
	}

	return 0;
}

int cdbm_index(struct cdb_make *cdbm, int (*idxfmt) (array * fmt,
		unsigned char *key, size_t ks), unsigned char *key, size_t ks)
{
	static array fmt;

	idxfmt(&fmt, key, ks);
	cdb_make_add(cdbm, (unsigned char *)fmt.p, array_bytes(&fmt), key, ks);
	array_reset(&fmt);
	return 0;
}

int cdb_idx_find(const char *file, int (*idxfmt) (array * fmt,
		unsigned char *key, size_t ks), unsigned char *key, size_t ks,
	array * keys)
{
	static array fmt, v;
	int err;

	idxfmt(&fmt, key, ks);
	err = cdb_get(file, fmt.p, array_bytes(&fmt), &v);
	return err;
}
/*  use nentry */

//#include <stdlib.h>
//int _cdb_get(struct cdb *result, char *key, size_t ks, array * v)
//{
//	int err, len;
//	array *newv;
//
//	err = cdb_find(result, (unsigned char *)key, ks);
//
//	if (err == 0) {
//		return 0;
//	} else if (err == -1) {
//		return -1;
//	} else {
//		do {
//			len = cdb_datalen(result);
//
//			newv = malloc(sizeof(array));
//			memset(newv, 0, sizeof(array));
//
//			/* introspection...;-( */
//			newv->p = malloc(len);
//			newv->allocated = len;
//			newv->initialized = len;
//
//			ops->add_key(tmp, key, klen);
//			ops->add_to_array(tmp, entries);
//
//			err = cdb_read(result, (unsigned char *)newv->p, len,
//				cdb_datapos(result));
//
//			if (err < 0) {
//				return -2;
//			}
//
//			array_catb(v, (char *)newv, sizeof(array));
//		} while (cdb_findnext(result, (unsigned char *)key, ks) > 0);
//
//	}
//	return 1;
//}
