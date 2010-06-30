#!/bin/bash

if [ "$1" == "" ]; then
	echo "This script sets up your blogs title, tagline and password"
	echo "Usage: ./setup.sh password tagline blogname"
	echo "Example ./setup.sh mytitle \"My tag line in quotes\" mypass"
	exit 1
fi

rm -f db/conf.inc
./blogcmd -a -b db/conf.inc -k input -v "$1"
./blogcmd -a -b db/conf.inc -k tagline -v "$2"
./blogcmd -a -b db/conf.inc -k title -v "$3"

echo "Created the file conf.inc. This file need to be in the same directory as your blog.cgi"

