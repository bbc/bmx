#!/bin/sh

base=$(dirname $0)
. $base/common.sh

run_test $@ test3 50 "0" 4
