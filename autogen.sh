#!/bin/sh -e

test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.

( cd "$srcdir" && intltoolize --copy --force --automake && autoreconf --force --install --verbose --warnings=all "$srcdir" )
test -n "$NOCONFIGURE" || "$srcdir/configure" "$@"
