#!/bin/sh

# check for libtoolize on Linux and glibtoolize on Mac
if command -v libtoolize >/dev/null 2>&1; then
  lt=libtoolize
elif command -v glibtoolize >/dev/null 2>&1; then
  lt=glibtoolize
else
  echo "ERROR: libtoolize or glibtoolize not found"
  exit 1
fi


set -x
aclocal -I m4
$lt
autoheader -f
automake --foreign --add-missing -c
autoconf
