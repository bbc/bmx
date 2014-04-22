#!/bin/sh

base=$(dirname $0)
. $base/common.sh

run_test $@ test1 "9,572" 4
