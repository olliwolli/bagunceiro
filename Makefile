.SUFFIXES: .adm.o .o .c
DIET=/opt/diet
DIET_INCLUDE=$(DIET)/include
CC=$(DIET)/bin/diet gcc

CFLAGS=-Wall -I$(DIET_INCLUDE) -g -DNO_ADMIN_MODE
LDFLAGS=-lowfat -L$(DIET)/lib -static

#CFLAGS=-Wall -I$(DIET_INCLUDE) -Os -fomit-frame-pointer
#LDFLAGS=-lowfat -L$(DIET)/lib -static -s

CFLAGS_ADMIN=-Wall -I$(DIET_INCLUDE) -DADMIN_MODE
LDFLAGS_ADMIN=$(LDFLAGS)

TARGETS=blog.cgi blogger blog.cgi.adm blog.cgi.strip

BLOG_O=z_mainblog.o z_blog.o z_entry.o z_time.o z_cdb.o z_format.o z_conf.o
BLOGGER_O=z_blogger.adm.o z_time.adm.o z_cdb.adm.o z_entry.adm.o 

all: $(TARGETS)
HEADERS=z_blog.h z_cdb.h z_conf.h z_entry.h z_features.h z_format.h z_time.h
SOURCES=z_blog.c z_cdb.c z_conf.c z_entry.c z_format.c z_mainblog.c z_time.c

BLOG_O_ADM=$(SOURCES:%.c=%.adm.o)

z_mainblog.o: z_blog.c z_blog.h z_time.h z_time.c z_features.h
z_blog.o: z_blog.c z_blog.h z_cdb.h z_cdb.c z_time.h z_entry.h z_entry.c z_format.c z_format.h z_features.h
z_blogger.o: z_blogger.c z_blog.h z_blog.c z_cdb.h z_cdb.c z_time.h z_time.c z_entry.h z_entry.c z_features.h
z_format.o: z_format.h z_format.c z_blog.h z_blog.c z_entry.h z_entry.c z_time.h z_time.c
z_cdb.o: z_cdb.h z_cdb.c z_entry.h z_entry.c z_features.h
z_time.o: z_time.h z_time.c z_features.h
z_entry.o: z_entry.h z_entry.c z_cdb.h z_cdb.c z_entry.h z_entry.c z_time.h z_time.c z_features.h

.c.adm.o : $(HEADERS)
	$(CC) -c -o $@ $(CFLAGS_ADMIN) $(<:.adm.o=.c)

z_blogger.adm.o: z_blogger.c
	$(CC) -c -o $@ $(CFLAGS_ADMIN) $(<:.adm.o=.c)

blog.cgi.adm: $(BLOG_O_ADM) 
	$(CC) -o $@ $(BLOG_O_ADM) $(LDFLAGS_ADMIN)

blog.cgi: $(BLOG_O)
	$(CC) -o $@ $(BLOG_O) $(LDFLAGS)

blog.cgi.strip: blog.cgi
	cp -p $^ $@
	-strip -R .note -R .comment $@

blogger: $(BLOGGER_O)
	$(CC) -o $@ $(BLOGGER_O) $(LDFLAGS)

clean:
	rm -f $(TARGETS) *~ *.o
