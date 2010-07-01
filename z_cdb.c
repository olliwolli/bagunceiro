#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>

#include <cdb_make.h>
#include <str.h>
#include <cdb.h>
#include <open.h>
#include <alloca.h>
#include <array.h>
#include <errno.h>

#include "z_features.h"
#include "z_time.h"
#include "z_cdb.h"
#include "z_entry.h"

inline static int exists(const char *file)
{
	if (access(file, F_OK | W_OK) != -1) {
		return 1;
	}
	return 0;
}

inline struct cdb *cdb_open_read(const char *file)
{
	struct cdb *result;
	result = malloc(sizeof(struct cdb));

	int fd = open_read(file);
	if (fd <= 0) {
		return result;
	}

	cdb_init(result, fd);
	return result;
}

inline void cdb_close(struct cdb *result)
{
	if (result->fd > 0) {
		close(result->fd);
	}
	cdb_free(result);
	free(result);
}

int do_day_index(array * fmt, const unsigned char *key, const size_t ks)
{
	char buf[FMT_TAIA_STR];
	struct taia l;
	int len;

	taia_unpack((char *)key, &l);

	len = fmt_time_str(buf, &l);

	array_catb(fmt, buf, len);
	array_cats(fmt, "@");

	return 0;
}

#ifdef ADMIN_MODE
typedef enum cop {
	OP_CPY, OP_MOD, OP_DEL, OP_ADD, OP_ADD_IDX, OP_DEL_IDX
} cop_t;

typedef enum mop {
	CP_NOP, CP_PAR, CP_SRC, CP_IDX
} mop_t;

inline struct cdb_make *cdb_open_write(const char *file)
{
	int fp;
	struct cdb_make *cdbm;

	cdbm = malloc(sizeof(struct cdb_make));
	fp = open_write(file);

	if (fp <= 0) {
		return NULL;
	}
	cdb_make_start(cdbm, fp);
	return cdbm;
}

inline void cdbm_make_close(struct cdb_make *cdbm)
{
	cdb_make_finish(cdbm);
	free(cdbm);
}



/* dirty */
static int __cdb_remake_real(const char *name, const char *newname,
	const unsigned char *skey, const
	size_t slen, unsigned char *value, size_t vlen, enum cop op)
{
	struct cdb_make *cdbm;
	struct cdb *c;
	int numrec = 0;
	int found = 0;
	int first;
	uint32 kpos, kp, klen, dp, dlen;
	enum mop loop_copy_src;

	unsigned char *key, *data;
	static array fmt;

	do_day_index(&fmt, skey, TAIA_PACK);

	cdbm = cdb_open_write(newname);
	c = cdb_open_read(name);

	if (c->fd <= 0)
		first = 0;
	else
		first = cdb_firstkey(c, &kpos);

	if (OP_ADD == op || OP_ADD_IDX == op) {
		cdb_make_add(cdbm, skey, slen, value, vlen);

		if (OP_ADD_IDX == op)	/* add an index */
			cdb_make_add(cdbm, (unsigned char *)fmt.p,
				array_bytes(&fmt), skey, TAIA_PACK);

		++numrec;

		if (first == -1 || first == 0)
			goto finish;
	}

	if (!first)
		goto finish;

	do {
		loop_copy_src = CP_SRC;

		kp = cdb_keypos(c);
		klen = cdb_keylen(c);
		key = alloca(klen);
		cdb_read(c, key, klen, kp);

		/* if the key is the same modifiy the entry */
		switch (op) {
		case OP_MOD:
			if (!memcmp
				((const char *)key, (const char *)skey, klen)) {
				found = 1;
				loop_copy_src = CP_PAR;
			}
			break;
		case OP_DEL_IDX:	/* fall-trough */
		case OP_DEL:
			if (!memcmp
				((const char *)key, (const char *)skey, klen)) {
				loop_copy_src = CP_NOP;
				found = 1;
			}

			/* remove index entry */
			if (op == OP_DEL_IDX && !memcmp(fmt.p, key, klen)) {
				loop_copy_src = CP_IDX;
			}

			break;
		default:
			break;
		}

		switch (loop_copy_src) {
		case CP_PAR:
			data = value;
			dlen = vlen;
			cdb_make_add(cdbm, key, klen, data, dlen);
			++numrec;
			break;
		case CP_NOP:
			break;
		case CP_SRC:
			/*  fall-through */
		default:
			++numrec;
			/* by default copy from src */
			dp = cdb_datapos(c);
			dlen = cdb_datalen(c);
			data = alloca(dlen);
			cdb_read(c, data, dlen, dp);

			if (CP_IDX == loop_copy_src) {
				if (!memcmp(data, skey, slen))	/* if the key is the value ?!?1 */
					continue;
			}

			cdb_make_add(cdbm, key, klen, data, dlen);
		}
	} while (cdb_nextkey(c, &kpos) == 1);

finish:
	cdb_close(c);
	cdbm_make_close(cdbm);

	if (op == OP_DEL || op == OP_MOD)
		if (!found) {
			errno = ENOKEY;
			return -2;
		}

	array_reset(&fmt);

	return numrec;
}

static int __cdb_remake(const char *file, const char *k, const size_t ks,
	const char *v, const size_t vs, enum cop op)
{
	int err;
	char tmpfile[str_len(file) + 5];

	str_copy(tmpfile, file);
	strcat(tmpfile, ".tmp");

	if (v != NULL) {
		err = __cdb_remake_real(file, tmpfile,
			(unsigned char *)k, ks, (unsigned char *)v, vs, op);
	} else {
		err = __cdb_remake_real(file, tmpfile,
			(unsigned char *)k, ks, NULL, 0, op);
	}

	rename(tmpfile, file);

	if (err == 0)
		unlink(file);

	return err;
}

/*  other */

int cdb_mod(const char *file, const char *k, const size_t ks, const char *v,
	const size_t vs)
{
	return __cdb_remake(file, k, ks, v, vs, OP_MOD);
}

int cdb_add(const char *file, const char *k, const size_t ks, const char *v,
	const size_t vs)
{
	return __cdb_remake(file, k, ks, v, vs, OP_ADD);
}

int cdb_add_idx(const char *file, const char *k, const size_t ks, const char *v,
	const size_t vs)
{
	return __cdb_remake(file, k, ks, v, vs, OP_ADD_IDX);
}

int cdb_del(const char *file, const char *k, const size_t ks)
{
	return __cdb_remake(file, k, ks, NULL, 0, OP_DEL);
}

int cdb_del_idx(const char *file, const char *k, const size_t ks)
{
	return __cdb_remake(file, k, ks, NULL, 0, OP_DEL_IDX);
}





#endif

inline int _cdb_get_value(const char * file, char *key, size_t ks, struct nentry *v)
{
	struct cdb * cdb;
	int err;
	memset(v, 0, sizeof(struct nentry));
	cdb = cdb_open_read(file);
	err = _cdb_get(cdb, key, ks , v);
	cdb_close(cdb);
	return err;
}


int _cdb_get(struct cdb *result, char *key, size_t ks, struct nentry *v)
{
	int err, dlen;

	unsigned char * buf;

	err = cdb_find(result, (unsigned char *)key, ks);

	if (err <= 0) {
		return err;
	} else {
		/* set key */
		v->kp = malloc(ks);
		memcpy(v->kp, key, ks);
		taia_unpack(key, &v->k);

		do {
			dlen = cdb_datalen(result);

			buf = alloca(dlen);

			err = cdb_read(result,
				buf,
				dlen, cdb_datapos(result));

			array_catb(&v->e, (char *)buf, dlen);

			if (err < 0)
				return -2;

		} while (cdb_findnext(result, (unsigned char *)key, ks) > 0);
	}
	return 1;
}
