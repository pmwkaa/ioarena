N1	?= 500000
N2	?= 1000000
N3	?= 10000

EXE	?= @BUILD/src/ioarena
DATA	?= /sandbox/test/@DATA.ioarena
CSV	?= @DATA.CSV
ENGINES	?= lmdb sophia forestdb leveldb rocksdb wiredtiger
LOGOUT	?= >>

.PHONY: all clean $(ENGINES)

all: $(ENGINES)

clean:
	rm -f *.log

$(EXE):
	./runme.sh

#######################################################################

define rule

# Первый прогон:
# - CRUD замера "стоимости" (ресурсов) и "качества" (задержка)
#
# Эти данные попадают:
#  - в слайд №3 "Рекорды"
#  - в cлайд №31 "LAZY + CRUD * 10**5"
#  - в слайд №34 "Стоимость" (замер ресурсов подобно time)
$(1): $(1).pass-1.log $(1).pass-2.log $(1).pass-3.log

$(1).pass-1.log: $(EXE)
	@ echo "$$@..." \
	&& rm -rf $(DATA) $(CSV)/1-$(1)*.csv run.txt \
	&& mkdir -p $(CSV) $(DATA) \
	&& $(EXE) -D $(1) -p $(DATA) -m lazy -n $(N1) -B crud -C $(CSV)/1- $(LOGOUT) run.txt \
	&& mv run.txt $$@ || (echo "failed $$@" > /dev/stderr && mv run.txt $$@.failed && exit 1)

# Второй прогон:
# - CRUD на 10**6 с не-синхронной записью
# - потом на этой базе показываем масштабирование по CPU
#
# Эти данные попадают:
# - в cлайд №32 "NOSYNC + CRUD * 10**6"
# - в слайд №29 "Разгоняется"
$(1).pass-2.log: $(EXE)
	@ echo "$$@..." \
	&& rm -rf $(DATA) $(CSV)/2*-$(1)*.csv run.txt \
	&& mkdir -p $(CSV) $(DATA) \
	&& $(EXE) -D $(1) -p $(DATA) -m nosync -n $(N2) -B crud -C $(CSV)/2a- $(LOGOUT) run.txt \
	&& $(EXE) -D $(1) -p $(DATA) -n $(N2) -B get -C $(CSV)/2b1- $(LOGOUT) run.txt \
	&& $(EXE) -D $(1) -p $(DATA) -n $(N2) -B get -r 2 -C $(CSV)/2b2- $(LOGOUT) run.txt \
	&& $(EXE) -D $(1) -p $(DATA) -n $(N2) -B get -r 4 -C $(CSV)/2b4- $(LOGOUT) run.txt \
	&& $(EXE) -D $(1) -p $(DATA) -n $(N2) -B get -r 8 -C $(CSV)/2b8- $(LOGOUT) run.txt \
	&& mv run.txt $$@ || (echo "failed $$@" > /dev/stderr && mv run.txt $$@.failed && exit 2)

# Третий прогон:
# - просто CRUD на 10000 с синхронной записью
#
# Эти данные попадают:
#  - в cлайд №30 "SYNC + CRUD * 10**4"
$(1).pass-3.log: $(EXE)
	@ echo "$$@..." \
	&& rm -rf $(DATA) $(CSV)/3-$(1)*.csv run.txt \
	&& mkdir -p $(CSV) $(DATA) \
	&& $(EXE) -D $(1) -p $(DATA) -m sync -n $(N3) -B crud -C $(CSV)/3- $(LOGOUT) run.txt \
	&& mv run.txt $$@ || (echo "failed $$@" > /dev/stderr && mv run.txt $$@.failed && exit 3)

endef

$(eval $(foreach 1,$(ENGINES),$(call rule)))
