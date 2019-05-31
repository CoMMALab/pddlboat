#!/bin/bash

CDIR=`dirname "$0"`
FDDIR=$CDIR/fd

if [ -d "$FDDIR" ]; then
    pushd $FDDIR
    hg update
    popd
else
    hg clone http://hg.fast-downward.org $FDDIR
fi
