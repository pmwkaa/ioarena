#!/bin/bash

echo "*** update submodule(s)..."
git submodule sync
git submodule update --init --recursive

echo "*** cleanup..."
git clean -x -f -d
git submodule foreach --recursive git clean -x -f -d

DIR=@BUILD
echo "*** run cmake..."
mkdir -p $DIR && \
	(cd $DIR && cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .. \
	-DENABLE_ROCKSDB=ON \
	-DENABLE_FORESTDB=ON \
	-DENABLE_SOPHIA=ON \
	-DENABLE_WIREDTIGER=ON \
	-DENABLE_LMDB=ON \
	-DENABLE_MDBX=ON \
	-DENABLE_LEVELDB=ON \
	-DENABLE_SQLITE3=ON \
	-DENABLE_EJDB=ON \
	-DENABLE_VEDISDB=ON \
	-DENABLE_IOWOW=ON \
	) || exit 1

echo "*** run make..."

# $@ for additional params like ./runme.sh VERBOSE=1 -j 8
make "$@" -C $DIR
