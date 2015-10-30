#!/bin/bash

failure() {
        echo "HL++2015: Oops, $* failed ;(" >&2
        exit 1
}

N=10000
EXE=@BUILD/src/ioarena
DATA=@DATA.DB
CSV=@DATA.CSV
if [ ! -x $EXE ]; then
	./runme.sh || failure 'build'
fi

ENGINES="lmdb sophia forestdb leveldb rocksdb wiredtiger mdbx"
#ENGINES=lmdb
#ENGINES=sophia
#ENGINES=debug

run() {
	for e in $ENGINES; do
		echo "$EXE -D $e -p $DATA $*"
		$EXE -D $e -p $DATA $* || failure "$EXE -D $e -p $DATA $*"
	done
}

#######################################################################

## Первый прогон:
## - делаем вид что импортируем данные (поэтому nosync)
## - далее подаем нагрузку и замеряем
##
## Эти данные попадают:
##  - в слайд №3 "Рекорды"
##  - в cлайд №31 "LAZY + CRUD * 10**5"
##  - в слайд №34 "Стоимость" (замер ресурсов через time)
rm -rf $DATA $CSV/pass1-*.csv && mkdir -p $CSV $DATA
run -m nosync -n ${N}0 -B set -C $CSV/pass1-
run -m lazy -n ${N}0 -B get,iterate,crud -w 1 -r 4 -C $CSV/pass1-


## Второй прогон:
## - просто CRUD на 10**6 с не-синхронной записью
## - потом на этой базе показываем масштабирование по CPU
##
## Эти данные попадают:
## - в cлайд №32 "NOSYNC + CRUD * 10**6"
## - в слайд №29 "Разгоняется"
rm -rf $DATA $CSV/pass2-*.csv && mkdir -p $CSV $DATA
run -m nosync -n ${N}00 -B set
run -m nosync -n ${N}00 -B get,iterate,crud -w 1 -r 4 -C $CSV/pass2-

## Третий прогон:
## - просто CRUD на 10000 с синхронной записью
##
## Эти данные попадают:
##  - в cлайд №30 "SYNC + CRUD * 10**4"
rm -rf $DATA $CSV/pass3-*.csv && mkdir -p $CSV $DATA
run -m nosync -n ${N} -B set
run -m sync -n ${N} -B get,iterate,crud -w 1 -r 4 -C $CSV/pass3-
