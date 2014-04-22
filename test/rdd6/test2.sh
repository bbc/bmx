#!/bin/sh

base=$(dirname $0)
. $base/common.sh

run_test $@ test2 "9,572" 4
