#!/bin/sh -e

test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.

mkdir -p m4
intltoolize --copy --force --automake && autoreconf --force --install --verbose --warnings=all "$srcdir"
test -n "$NOCONFIGURE" || "$srcdir/configure" "$@"
