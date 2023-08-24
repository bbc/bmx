#!/bin/sh

set -e

COMMAND=$1
TESTFILE=$2
WITH_VALGRIND=$3

CMD_PREFIX=
if [ "$WITH_VALGRIND" = "ON" ]
then
  CMD_PREFIX="valgrind --leak-check=full -q --error-exitcode=1"
fi

$CMD_PREFIX "$COMMAND" "$TESTFILE"
$CMD_PREFIX "$COMMAND" stdin <"$TESTFILE"
cat "$TESTFILE" | $CMD_PREFIX "$COMMAND" stdin
$CMD_PREFIX "$COMMAND" stdout >"$TESTFILE"
$CMD_PREFIX "$COMMAND" stdout | $CMD_PREFIX "$COMMAND" stdin
