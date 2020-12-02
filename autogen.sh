#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="iog"

(test -f $srcdir/configure.ac) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

which gde2-autogen || {
    echo "You need to install gde2-common"
    exit 1
}

which yelp-build || {
    echo "You need to install yelp-tools"
    exit 1
} 

REQUIRED_AUTOMAKE_VERSION=1.9
GDE2_DATADIR="$gde2_datadir"

. gde2-autogen

