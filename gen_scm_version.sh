#!/bin/sh
# Based on patch from Peter Hutterer sent to X.Org
#    http://lists.x.org/archives/xorg-devel/2010-April/007415.html
#
# Get the git version if possible and store it in $SCM_VER_FILE if it differs to
# the one contained already.

SCM_VER_FILE="bmx_scm_version.h"

GIT_VER=""
if command -v git >/dev/null 2>&1; then
  git update-index --refresh --unmerged -q >/dev/null 2>&1
  GIT_VER=`git describe 2>/dev/null`
  if test -n "$GIT_VER" && ! git diff-index --quiet HEAD --; then
    GIT_VER="$GIT_VER"-dirty
  fi
fi
if test -z "$GIT_VER"; then
  exit 0
fi

OUTSTR="#define BMX_SCM_VERSION \"$GIT_VER\""

if test -e "$SCM_VER_FILE"; then
  EXISTING_STR=`cat $SCM_VER_FILE`
  if test "$EXISTING_STR" = "$OUTSTR"; then
    exit 0
  fi
fi

echo "$OUTSTR" > $SCM_VER_FILE

