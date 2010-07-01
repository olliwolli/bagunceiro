#define CDB_DO_COPY 0
#define CDB_DO_NOT_COPY 1

	/* ten should be ok for now, if not you can increase
	 * that or implement it as with the dynamic array
	 * interface */
#define NUM_CALLBACKS 10

typedef struct cdb_action{
	struct cdb_make *w;
	struct cdb *r;
	char *f; /* file to read from */
	char *t; /* file to write to */
	char *k;
	size_t ks;
	char *v;
	size_t vs;
	struct nentry e;
	unsigned char *needle[NUM_CALLBACKS];
	int (*cbs[NUM_CALLBACKS])(struct cdb_action * a, int * copied);
	int cbsnum;
} t_cdb_action;
