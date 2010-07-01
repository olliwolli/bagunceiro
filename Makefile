.SUFFIXES: .adm.o .o .c
DIET=/opt/diet
DIET_INCLUDE=$(DIET)/include
LIB=lib
DBIN=$(DIET)/bin/diet
CC=$(DBIN) gcc

# dynamic and static content in the same path
INSTALL_DIR=/var/www/dynamic/blog
BINDIR=/usr/bin
DBDIR=$(INSTALL_DIR)/db
TINYDIR=$(INSTALL_DIR)/tinyeditor
WEBUSER=www-data
WEBGRP=www-data

CFLAGS=-Wall -I$(DIET_INCLUDE)
LDFLAGS=-lowfat -L$(DIET)/$(LIB) -static
LDFLAGS_ADMIN=$(LDFLAGS)

ifneq ($(DEBUG),)
CFLAGS+=-g
CFLAGS_ADMIN=$(CFLAGS) -DADMIN_MODE -DADMIN_MODE_PASS -g
else
CFLAGS+=-Os -fomit-frame-pointer
LDFLAGS+=-s 
CFLAGS_ADMIN=$(CFLAGS) -DADMIN_MODE -DADMIN_MODE_PASS
endif

TARGETS=blog.cgi blogcmd admin.cgi

BLOG_O=z_mainblog.o z_blog.o z_entry.o z_time.o z_cdb.o z_format.o z_conf.o z_day_entry.o z_comment.o z_html5.o z_http.o z_rss.o
BLOGGER_O=z_blogger.adm.o z_time.adm.o z_cdb.adm.o z_entry.adm.o z_day_entry.o z_comment.o z_html5.o z_http.o z_rss.o
all: $(TARGETS)

HEADERS=z_blog.h z_cdb.h z_conf.h z_entry.h z_features.h z_format.h z_time.h z_day_entry.h z_comment.h z_html5.h z_http.h z_rss.h
SOURCES=z_blog.c z_cdb.c z_conf.c z_entry.c z_format.c z_mainblog.c z_time.c z_day_entry.c z_comment.c z_html5.c z_http.c z_rss.c


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

_admin: $(BLOG_O_ADM) 
	$(CC) -o $@ $(BLOG_O_ADM) $(LDFLAGS_ADMIN)

_blog: $(BLOG_O)
	$(CC) -o $@ $(BLOG_O) $(LDFLAGS)

blog.cgi: _blog
	cp -p $^ $@
	-strip -R .note -R .comment $@
	
admin.cgi: _admin
	cp -p $^ $@
	-strip -R .note -R .comment $@

blogcmd: $(BLOGGER_O)
	$(CC) -o $@ $(BLOGGER_O) $(LDFLAGS)
	-strip -R .note -R .comment $@

clean:
	rm -f _* $(TARGETS) *~ *.o cscope.out tags

install:
# create used directories
	install -d $(INSTALL_DIR)
	install -d $(BIN_DIR)
	install -d -o $(WEBUSER) -g $(WEBGRP) $(INSTALL_DIR)/img
	install -d -o $(WEBUSER) -g $(WEBGRP) $(INSTALL_DIR)/db

# initial database files
	install -o $(WEBUSER) -g $(WEBGRP) db.init $(INSTALL_DIR)/db/db.inc
	install -o $(WEBUSER) -g $(WEBGRP) session.init $(INSTALL_DIR)/db/session.inc
	
# wysiwyg editor
	install -d $(TINYDIR) $(TINYDIR)/images
	install tinyeditor/style.css $(TINYDIR)
	install tinyeditor/packed.js $(TINYDIR)
	install tinyeditor/images/* $(TINYDIR)/images

# upload functionality
	install upload.pl $(INSTALL_DIR)
	install upload.js $(INSTALL_DIR)

# install cgi scripts
	install blog.cgi $(INSTALL_DIR)
	install admin.cgi $(INSTALL_DIR)

# command line configuration utility
	install blogcmd $(BIN_DIR)

# css stylesheets
	install *css $(INSTALL_DIR)
