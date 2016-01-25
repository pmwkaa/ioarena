This is the HL++2015 branch
===========================

Here is the script `HL2015.mk` that was used to benchmarking
for speech/presentation about LMDB & MDBX
on the Highload++2015 Conference in Moscow/Russia.

This script is publicly available to any interested
could reproduce the benchmark results. Also the script may
be a good example of how you can use ioarena.

Original title of speech/presentation in russian
is "Движок LMDB - особенный чемпион", that could be translated
as "LMDB engine - is the bizarre champion".

For more information please see:
* [abstract in russian](http://www.highload.ru/2015/abstracts/1831.html)
* [translation by Yandex](https://translate.yandex.ru/translate?url=http://www.highload.ru/2015/abstracts/1831.html)
* [translation by Google](https://translate.google.com/translate?sl=ru&tl=en&u=www.highload.ru%2F2015%2Fabstracts%2F1831.html)
* [slides](http://www.slideshare.net/leoyuriev/lmdb)
* [about Highload++](http://highload.co/)


=======================================================================


**ioarena** - embedded storage benchmarking
-------------------------------------------

<img src="https://travis-ci.org/pmwkaa/ioarena.svg?branch=master" />

**ioarena** is an utility designed for evaluating performance
of embedded databases.

The goal of this project is to provide a standart and simple
in use instrument for benchmarking, so any database developer or user
can reference to or repeat obtained results.

Benchmarking methods: *set*, *get*, *delete*, *iterate*, *batch*, *CRUD*

Sync modes: *sync*, *lazy*, *no-sync*

WAL modes: *indef* (per engine default), *wal-on*, *wal-off*

Supported databases: **rocksdb**, **leveldb**, **forestdb**, **lmdb**,
**mdbx** (modified lmdb), ~~nessdb~~, **wiredtiger**, **sophia**

*New drivers or any kind of enhancements are very welcome!*

Usage
-----

```sh
IOARENA (embedded storage benchmarking)

usage: ioarena [hDBCpnkvmlrwic]
  -D <database_driver>
     choices: sophia, leveldb, rocksdb, wiredtiger, forestdb, lmdb, mdbx, dummy
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
