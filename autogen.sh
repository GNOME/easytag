#!/bin/sh

# New version from 18/08/2005
libtoolize --force --copy
aclocal
autoheader
automake --add-missing -a --foreign
autoconf


# Old version
#gettextize -c
#libtoolize -c
#aclocal
#autoconf
#autoheader
#automake -c -a --foreign
