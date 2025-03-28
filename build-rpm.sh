#!/bin/sh

RPMBUILDROOT="${PWD}/rpmbuild"

FILE_VERSION="src/version.h"

# Get current version
VERSION_MAJOR=`sed -n 's/^\#define\s\+APP_VERSION_MAJOR\s\+\([0-9]\+\)/\1/p' $FILE_VERSION`
VERSION_MINOR=`sed -n 's/^\#define\s\+APP_VERSION_MINOR\s\+\([0-9]\+\)/\1/p' $FILE_VERSION`
VERSION="$VERSION_MAJOR.$VERSION_MINOR"

for d in BUILD  BUILDROOT  RPMS  SOURCES  SPECS  SRPMS; do
    mkdir -p $RPMBUILDROOT/$d || exit 1
done

# export the source into SOURCES
mkdir $RPMBUILDROOT/SOURCES/spivak-$VERSION || exit 1
git archive master | tar -x -C $RPMBUILDROOT/SOURCES/spivak-$VERSION
(cd $RPMBUILDROOT/SOURCES && tar zcf spivak-$VERSION.tar.gz spivak-$VERSION)
rm -rf $RPMBUILDROOT/SOURCES/spivak-$VERSION

rpmbuild --define "_topdir $RPMBUILDROOT"  -bb packaging/spivak.spec
