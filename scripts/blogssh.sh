#!/bin/bash

SERV=user@server.com
BCMD=../blogcmd
BLOG=../
DB=$REMOTE_BLOG/db/db.inc

cat /dev/stdin | ssh $SERV $BCMD -b $DB -i -n -t
