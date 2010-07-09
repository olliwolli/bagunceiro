#ifndef _Z_CDBNEW_H
#define _Z_CDBNEW_H

#include <uint32.h>
#include <array.h>

typedef struct nentry {
	array e;		/*  entry */
	array k;
} nentry_t;

/* ten should be ok for now, if not you can increase
 * that or implement it as with the dynamic array
 * interface */
#define NUM_OP 10


enum opt { O_NONE, O_ADD, O_DEL, O_ADD_MOD, O_MOD, O_FIND};

/* operation */
struct op {
	array n;  /* key string */
	array v;  /* value string */

	int kf; /* key found */
	int vf; /* value found */

	unsigned char *sv; /* scanned value */
	size_t svs;

	enum opt t;
};

typedef int (*filter)(unsigned char * k, size_t ks,
		unsigned char *v, size_t vs);


/* cdb write batch */
typedef struct cdbb{
	struct cdb_make *w; /* write */
	struct cdb *r;      /* read */
	uint32 kfindpos;

	char *f; /* file to read from */
	char *t; /* file to write to */

	struct op ops[NUM_OP]; /* operations */
	size_t tnum; /* number of operations */

	filter fil[NUM_OP]; /* filters */
	size_t fnum; /* number of filters */
} t_cdb_action;

void init_nentry(struct nentry *n);
void reset_nentry(struct nentry *n);
struct nentry * new_nentry(void);
void free_nentry(struct nentry *n);
int cdbb_apply(struct cdbb * a);
int cdbb_start_mod(struct cdbb * a, char * f);

void cdbb_add_filter(struct cdbb *a, filter cb);
void cdbb_add_op(struct cdbb *a, char * n, size_t ns, enum opt t);
void cdbb_add_vop(struct cdbb *a, char * n,	size_t ns, char *v, size_t vs, enum opt t);

/* HIGH LEVEL OPERATIONS */
void cdbb_add(struct cdbb *a, char *k, size_t ks, char *v, size_t vs);
void cdbb_mod(struct cdbb *a, char *k, size_t ks, char *v, size_t vs);
void cdbb_rep(struct cdbb *a, char *k, size_t ks, char *v, size_t vs);
void cdbb_del(struct cdbb *a, char *k, size_t ks);
void cdbb_del_val(struct cdbb *a, char *k, size_t ks, char *v, size_t vs);
int cdbb_readn(struct cdbb *a, char *k, size_t ks, char *v, size_t n);
int cdbb_read_nentry(struct cdbb *a, char *k, size_t ks, struct nentry *n);

int cdbb_open_read(struct cdbb *a, const char *f);
int cdbb_open_write(struct cdbb *a, const char *f);
void cdbb_close_write(struct cdbb *a);
void cdbb_close_read(struct cdbb *a);

int cdbb_firstkey(struct cdbb * a);
int cdbb_findnext(struct cdbb * a, const char *needle, struct nentry *n);

#endif
