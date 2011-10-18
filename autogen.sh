#!/bin/sh

# check for libtoolize on Linux and glibtoolize on Mac
if command -v libtoolize \&>/dev/null; then
  lt=libtoolize
elif command -v glibtoolize \&>/dev/null; then
  lt=glibtoolize
else
  echo "ERROR: libtoolize or glibtoolize not found"
  exit 1
fi


set -x
aclocal
$lt
autoheader -f
automake --foreign --add-missing -c
autoconf
