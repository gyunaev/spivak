#!/bin/sh

RPMBUILDROOT="${PWD}/rpmbuild"

VERSION=2.0

for d in BUILD  BUILDROOT  RPMS  SOURCES  SPECS  SRPMS; do
    mkdir -p $RPMBUILDROOT/$d || exit 1
done

# export the source into SOURCES
mkdir $RPMBUILDROOT/SOURCES/spivak-$VERSION || exit 1
git archive master | tar -x -C $RPMBUILDROOT/SOURCES/spivak-$VERSION
(cd $RPMBUILDROOT/SOURCES && tar zcf spivak-$VERSION.tar.gz spivak-$VERSION)
rm -rf $RPMBUILDROOT/SOURCES/spivak-$VERSION

rpmbuild --define "_topdir $RPMBUILDROOT"  -bb packaging/spivak.spec
