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
	-DENABLE_ROCKSDB=OFF \
	-DENABLE_FORESTDB=OFF \
	-DENABLE_SOPHIA=OFF \
	-DENABLE_WIREDTIGER=OFF \
	-DENABLE_LMDB=OFF \
	-DENABLE_MDBX=ON \
	-DENABLE_LEVELDB=OFF \
	-DENABLE_NESSDB=OFF \
	-DENABLE_SQLITE3=OFF \
	-DENABLE_EJDB=OFF \
	-DENABLE_VEDISDB=OFF \
	) || exit 1

echo "*** run make..."

# $@ for additional params like ./runme.sh VERBOSE=1 -j 8
make "$@" -C $DIR
