.SUFFIXES : .c .o 
DIET=/opt/diet
DIET_INCLUDE=$(DIET)/include
CFLAGS=-g -Wall -I$(DIET_INCLUDE)
LDFLAGS+=-lowfat -L$(DIET)/lib -static
CC=$(DIET)/bin/diet gcc

BLOG_O=z_mainblog.o z_blog.o z_entry.o z_time.o  z_cdb.o format.o
BLOGGER_O=z_time.o z_cdb.o z_entry.o z_blogger.o

SRCS=$(OBJS:.o=.c)
TARGETS=blog.cgi blogger

all: $(TARGETS)
format.o: format.c format.h

z_cdb.o: z_cdb.c z_cdb.h z_features.h
z_time.o: z_time.h z_time.c z_features.h
z_entry.o: z_entry.c z_entry.h z_cdb.h z_cdb.c z_features.h
z_blog.o: z_blog.c z_blog.h z_time.h z_entry.h z_features.h

z_mainblog.o: z_blog.c z_blog.h z_features.h
z_blogger.o: z_blogger.c z_cdb.c z_time.c z_entry.c z_features.h

blog.cgi: $(BLOG_O)
	$(CC) -o blog.cgi $(BLOG_O) $(LDFLAGS)

blogger: $(BLOGGER_O)
	$(CC) -o blogger $(BLOGGER_O) $(LDFLAGS)

clean:
	rm -f $(BLOGGER_O) $(BLOG_O) $(TARGETS) *~  *.o
