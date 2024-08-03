#!/bin/sh

set -eu
TMPDIR_="${TMPDIR:-/tmp}/$(mktemp -u XXXXXXXXXX)"
mkdir "$TMPDIR_" -m 700
trap 'rm "$TMPDIR_/ipconf; rmdir "$TMPDIR_"' EXIT

(
COUNTER=0
while [ "$COUNTER" -lt 256 ]; do
	printf 'route add "%s:%x00:0:0:0:0/64" via "fe80::100:%x" dev "%s"\n' "$1" "$COUNTER" "$COUNTER" "$2"
	COUNTER="$((COUNTER+1))"
done
) > "$TMPDIR_/ipconf"
ip -b "$TMPDIR_/ipconf"
