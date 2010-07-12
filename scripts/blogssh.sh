#!/bin/bash

SERV=user@server.com
BCMD=/home/mludi/clog/blogcmd
BLOG=/var/www/dynamic/blog/
DB=$REMOTE_BLOG/db/db.inc

cat /dev/stdin | ssh $SERV $BCMD -b $DB -i -n -t
