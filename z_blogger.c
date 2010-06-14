#include <unistd.h>

#include <array.h>
#include <str.h>
#include <byte.h>
#include <taia.h>

#include "z_features.h"
#include "z_cdb.h"
#include "z_entry.h"
#include "z_blog.h"
#include "z_time.h"

int load_config(blog_t * conf)
{
	return 0;
}

void read_stdin(array * content)
{
	while (buffer_get_array(buffer_0, content)) {
	}
}

void print_help()
{
	eprintf("Usage: cblog [OPTION]\n"
		"Reads value from stdin\n"
		"  -b path     db path (default: db/)\n"
		"  -a          add value read from stdin\n"
		"  -n key      add value read from stdin with given key\n"
		"  -d key      delete key\n"
		"  -m key      modify key to value from stdin\n"
		"  -s key      show entry\n"
		"  -z file     dump database file\n"
		"  -h          print help\n");
}

/* options */
int main(int argc, char **argv)
{
	static array pkey, fmtkey;
	static struct nentry entry;
	struct taia key;
	char c;
	int err = 0;
	char db[32] = "db/";

	while ((c = getopt(argc, argv, "b:ad:m:n:s:z:?h")) != -1) {
		switch (c) {
		case 'b':
			if (str_len(db) > 31) {
				eprintf("DB can only be 31 characters long\n");
				return 1;
			}
			str_copy(db, optarg);
			break;
		case 'a':
			read_stdin(&entry.e);

			err = add_entry_now(db, &entry);
			entry_dump(&entry);
			break;
		case 'n':
			if (str_len(optarg) != 32) {
				eprintf("Key must by 32 hex characters wide\n");
				return 2;
			}
			array_cats0(&pkey, optarg);
			scan_time_hex(&pkey, &key);
			entry.k = key;
			read_stdin(&entry.e);

			err = add_entry(db, &entry);
			entry_dump(&entry);
			break;
		case 'd':
			if (str_len(optarg) != 32) {
				eprintf("Key must by 32 hex characters wide\n");
				return 2;
			}

			array_cats0(&pkey, optarg);
			scan_time_hex(&pkey, &key);
			entry.k = key;

			err = delete_entry(db, &entry);
			break;
		case 'm':
			if (str_len(optarg) != 32) {
				eprintf("Key must by 32 hex characters wide\n");
				return 2;
			}

			array_cats0(&pkey, optarg);
			scan_time_hex(&pkey, &key);
			entry.k = key;
			read_stdin(&entry.e);

			err = modify_entry(db, &entry);
			break;
		case 's':
			if (str_len(optarg) != 32) {
				eprintf("Key must by 32 hex characters wide\n");
				return 2;
			}

			array_cats0(&pkey, optarg);
			scan_time_hex(&pkey, &key);
			entry.k = key;

			show_entry(db, &entry);
			fmt_time_hex(&fmtkey, &entry.k);
			eprintmf(fmtkey.p, ":", entry.e.p, "\n");
			break;
		case 'z':
			str_copy(db, optarg);

			break;
		default:
			print_help();
		}
	}
	return err;
}
