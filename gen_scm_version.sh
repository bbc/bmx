#!/bin/sh
# Create or modify the scm header file. The define in the header file
# identifies the git commit that corresponds to the source code.
#
# Based on patch from Peter Hutterer sent to X.Org
#    http://lists.x.org/archives/xorg-devel/2010-April/007415.html


scm_header=$1
dist_scm_header=$2
def_name=$3


# for the case of out of tree builds
abs_top_srcdir="$(cd "$(dirname "$0")" && pwd)"
export GIT_DIR=${abs_top_srcdir}/.git
export GIT_WORK_TREE=${abs_top_srcdir}/

git_ver=""
if command -v git >/dev/null 2>&1; then
  git update-index --refresh --unmerged -q >/dev/null 2>&1
  git_ver=`git describe 2>/dev/null`
  if test -n "$git_ver" && ! git diff-index --quiet HEAD --; then
    git_ver="$git_ver"-dirty
  fi
fi

define_str="#define $def_name \"$git_ver\""


# if $scm_header does not exist then default to the header provided by the
# distribution package, $dist_scm_header
if ! test -e $scm_header && test -e $dist_scm_header; then
  cp $dist_scm_header $scm_header
fi


if test -e "$scm_header"; then
  if test -z "$git_ver"; then
    # keep existing scm version file
    exit 0
  fi
  existing_define_str=`cat $scm_header`
  if test "$existing_define_str" = "$define_str"; then
    # no change and therefore don't modify scm version file
    exit 0
  fi
fi


echo "$define_str" > $scm_header

