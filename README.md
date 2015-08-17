
**ioarena** - embedded storage benchmarking
-------------------------------------------

<img src="https://travis-ci.org/pmwkaa/ioarena.svg?branch=master" />

**ioarena** is an utility designed for evaluating performance
of embedded databases.

The goal of this project is to provide a standart and simple
in use instrument for benchmarking, so any database developer or user
can reference to or repeat obtained results.

Benchmarking methods: *set*, *get*, *iterate*, *batch*, *transaction*

Supported databases: **rocksdb**, **leveldb**, **forestdb**, **lmdb**,
**wiredtiger**, **sophia**

*New drivers or any kind of enhancements are very welcome!*

usage
-----

```sh
IOARENA (embedded storage benchmarking)

usage: ioarena [hDBCpnkv]

  -D <database_driver>
	   sophia, leveldb, rocksdb, wiredtiger, forestdb, lmdb
  -B <benchmarks>
	   set, get, delete, iterate
	   batch, transaction
  -C generate csv                 
  -p <path>                   (./_ioarena)
  -n <number_of_operations>   (1000000)
  -k <key_size>               (16)
  -v <value_size>             (32)
  -h

  example:
	 ioarena -D rocksdb -T set,get -n 100000000
```

build
-----

```sh
git clone --recursive https://github.com/pmwkaa/ioarena
```

**cmake** is required for building.

To enable a specific database driver, pass -DENABLE\_**NAME**=ON to cmake.
If a specified database is not installed in system, it will be build from db/*name* directory.

```sh
mkdir build
cd build
cmake .. -DENABLE_ROCKSDB=ON
make
src/ioarena -h
```
