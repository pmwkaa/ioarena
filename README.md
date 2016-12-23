
**ioarena** - embedded storage benchmarking
-------------------------------------------

<img src="https://travis-ci.org/pmwkaa/ioarena.svg?branch=master" />

**ioarena** is an utility designed for evaluating performance
of embedded databases.

The goal of this project is to provide a standart and simple
in use instrument for benchmarking, so any database developer or user
can reference to or repeat obtained results.

Benchmarking methods: *set*, *get*, *delete*, *iterate*, *batch*, *crud*

Sync modes: *sync*, *lazy*, *no-sync*

WAL modes: *indef* (per engine default), *wal-on*, *wal-off*

Supported databases: **rocksdb**, **leveldb**, **forestdb**, **lmdb**,
**mdbx**, **nessdb**, **wiredtiger**, **sophia**, **sqlite3**, **ejdb**

*New drivers or any kind of enhancements are very welcome!*

Usage
-----

```sh
IOARENA (embedded storage benchmarking)

usage: ioarena [hDBCpnkvmlrwic]
  -D <database_driver>
     choices: sophia, leveldb, rocksdb, wiredtiger, forestdb, lmdb, mdbx, sqlite3, ejdb, dummy
  -B <benchmarks>
     choices: set, get, delete, iterate, batch, crud
  -m <sync_mode>                     (default: lazy)
     choices: sync, lazy, nosync
  -l <wal_mode>                      (default: indef)
     choices: indef, walon, waloff
  -C <name-prefix> generate csv      (default: (null))
  -p <path> for temporaries          (default: ./_ioarena)
  -n <number_of_operations>          (default: 1000000)
  -k <key_size>                      (default: 16)
  -v <value_size>                    (default: 32)
  -c continuous completing mode      (default: no)
  -r <number_of_read_threads>        (default: 0)
     `zero` to use single main/common thread
  -w <number_of_crud/write_threads>  (default: 0)
     `zero` to use single main/common thread
  -i ignore key-not-found error      (default: no)
  -h                                 help

example:
   ioarena -m sync -D sophia -B crud -n 100000000
```

Build
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

Authors
-------

| Name | Contribution |
|---|---|
| Dmitry Simonenko @pmwkaa | Original author. |
| Leonid Yuriev @leo-yuriev | Multithreading and isolation from the testcases the interface of a DB drivers cardinally redesigned, it is clear and intelligible now. |
| Bohu Tang @bohutang | Added support for NessDB. |
| Egor Zyryanov @er0p | Added support for SQLite, EJDB, Vedis. |
