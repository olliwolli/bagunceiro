# uses dietlibc (default)
# be sure that libowfat is compiled with the same
# libc as the one you are using here
WANT_DIET=yes

# compile fast cgi version
# increases the binary by about 36kb 
#WANT_FCGI=yes

# compile with clang instead of gcc
#WANT_CLANG=yes

# enable debugging
#DEBUG=yes

# makefile
.SUFFIXES: .std.o .adm.o .o .c

ifneq ($(WANT_DIET),)
DIET=/opt/diet
DIET_INCLUDE=$(DIET)/include
DBIN=$(DIET)/bin/diet
CFLAGS+=-I$(DIET_INCLUDE)
LDFLAGS+=-L$(DIET)/lib
else
LOWFAT_LIBC_INCLUDE=/opt/libclibowfat
LOWFAT_LIBC_LIB=/opt/libclibowfat
CFLAGS+=-I$(LOWFAT_LIBC_INCLUDE)
LDFLAGS+=-L$(LOWFAT_LIBC_LIB)
endif

ifneq ($(WANT_CLANG),)
CC=$(DBIN) clang
else
CC=$(DBIN) gcc
endif

# dynamic and static content in the same path
INSTALL_DIR=/var/www/blog
BINDIR=/usr/bin
DBDIR=$(INSTALL_DIR)/db
TINYDIR=$(INSTALL_DIR)/tinymce
WEBUSER=www-data
WEBGRP=www-data

# ok this might look a bit complicated, but rest assured
# the way to hell was paved with good intentions.
# if someone has a neat way to do this please mail me.
# basically we need the same object files with minor
# adjustments for 3 binaries and do not want to bloat
# binaries with unneeded stuff. the root of this evil
# is conditional compilation (ifdefs) in the code
#
#                              /       /
#                           .'<_.-._.'<
#                          /           \      .^.
#        ._               |  -+- -+-    |    (_|_)
#     r- |\                \   /       /      // 
#   /\ \\  :                \  -=-    /       \\
#    `. \\.'           ___.__`..;._.-'---...  //
#      ``\\      __.--"        `;'     __   `-.  
#        /\\.--""      __.,              ""-.  ".
#        ;=r    __.---"   | `__    __'   / .'  .'
#        '=/\\""           \             .'  .'
#            \\             |  __ __    /   |
#             \\            |  -- --   //`'`'
#              \\           |  -- --  ' | //
#               \\          |    .      |// AsH
#
# For example:
# admin.cgi includes write functionality of z_cdbb.c
# while blog.cgi does not include this functionality
# in order to have a smaller binary and strip unneeded
# functions. This however means that we need to compile 
# two versions of z_cdbb.o one, with write functions
# and one without them.
#
# Thats what the (pre/suf)fixes ADMIN/adm and STD/std
# are for.

CFLAGS+=-Wall
LDFLAGS+=-lowfat
LDFLAGS_ADMIN=$(LDFLAGS)

# static breaks valgrind leak checking
ifneq ($(DEBUG),)
CFLAGS+=-g3 -fomit-frame-pointer
CFLAGS_ADMIN=$(CFLAGS) -DADMIN_MODE -DADMIN_MODE_PASS -g
else
CFLAGS+=-Os -fomit-frame-pointer
LDFLAGS+=-s -static
CFLAGS_ADMIN=$(CFLAGS) -DADMIN_MODE -DADMIN_MODE_PASS
endif

TARGETS=blog.cgi admin.cgi blogcmd

all: $(TARGETS)

HEADERS=\
z_blog.h z_conf.h z_entry.h z_features.h z_format.h z_time.h z_day.h \
z_html5.h z_http.h z_rss.h z_cdbb.h z_result.h z_fmthtml.h z_fmtrss.h z_debug.h

SOURCES=z_blog.c z_conf.c z_entry.c z_format.c z_time.c z_day.c z_html5.c\
 z_http.c z_rss.c z_cdbb.c z_result.c z_fmthtml.c z_fmtrss.c z_debug.c

BLOG_O=$(SOURCES:%.c=%.o)
BLOG_O_STD=$(SOURCES:%.c=%.std.o)
BLOG_O_ADM=$(SOURCES:%.c=%.adm.o)

CFLAGS_STD:=$(CFLAGS)
ifneq ($(WANT_FCGI),)
CFLAGS+=-DWANT_FAST_CGI
FCGI_O=fcgiapp.o os_unix.o
BLOG_O+=$(FCGI_O)
BLOG_O_ADM+=$(FCGI_O)
endif

.c.std.o : $(HEADERS)
	$(CC) -c -o $@ $(CFLAGS_STD) $(<:.std.o=.c)

blogcmd: $(BLOG_O_ADM) z_blogger.adm.o
	$(CC) -o $@ $^ $(LDFLAGS_ADMIN)
	#-strip -R .note -R .comment $@

.c.adm.o : $(HEADERS)
	$(CC) -c -o $@ $(CFLAGS_ADMIN) $(<:.adm.o=.c)
 
_admin: $(BLOG_O_ADM) z_mainblog.adm.o
	$(CC) -o $@ $^ $(LDFLAGS_ADMIN)
	
admin.cgi: _admin
	cp -p $^ $@
	-strip -R .note -R .comment $@

_blog: $(BLOG_O) z_mainblog.o
	$(CC) -o $@ $^ $(LDFLAGS)

blog.cgi: _blog
	cp -p $^ $@
	-strip -R .note -R .comment $@

clean:
	rm -f _* $(TARGETS) css/*~ *~ *.o cscope.out tags

###########
# Deployment 

install:
# create used directories
	install -d $(INSTALL_DIR)
	install -d $(BIN_DIR)
	install -d -o $(WEBUSER) -g $(WEBGRP) $(INSTALL_DIR)/img
	install -d -o $(WEBUSER) -g $(WEBGRP) $(INSTALL_DIR)/db

# initial database files
	install -o $(WEBUSER) -g $(WEBGRP) init/db.inc $(INSTALL_DIR)/db/db.inc
	install -o $(WEBUSER) -g $(WEBGRP) init/session.inc $(INSTALL_DIR)/db/session.inc
	install -o $(WEBUSER) -g $(WEBGRP) init/conf.inc $(INSTALL_DIR)/db/conf.inc
	
# wysiwyg editor
	install -d $(TINYDIR)

# upload functionality
	install upload/upload.pl $(INSTALL_DIR)
	install upload/upload.js $(INSTALL_DIR)

# install cgi scripts
	install blog.cgi $(INSTALL_DIR)
	install admin.cgi $(INSTALL_DIR)

# command line configuration utility
	install blogcmd $(BIN_DIR)

# css stylesheets
	install css/*css $(INSTALL_DIR)
	install background.jpg $(INSTALL_DIR)
