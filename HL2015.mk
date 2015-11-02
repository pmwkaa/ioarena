N	?= 10000
EXE	?= @BUILD/src/ioarena
DATA	?= /sandbox/test/@DATA.ioarena
CSV	?= @DATA.CSV
ENGINES	?= lmdb sophia forestdb leveldb rocksdb wiredtiger
LOGOUT	?= >>
#LOGOUT  := | tee -a

.PHONY: all clean $(ENGINES)

all: $(ENGINES)

clean:
	rm -f *.log

$(EXE):
	./runme.sh

#######################################################################

define rule

# Первый прогон:
# - делаем вид что импортируем данные (поэтому nosync)
# - далее подаем нагрузку и замеряем
#
# Эти данные попадают:
#  - в слайд №3 "Рекорды"
#  - в cлайд №31 "LAZY + CRUD * 10**5"
#  - в слайд №34 "Стоимость" (замер ресурсов через time)
$(1): $(1).pass-1.log $(1).pass-2.log $(1).pass-3.log

$(1).pass-1.log: $(EXE)
	@ echo "$$@..." \
	&& rm -rf $(DATA) $(CSV)/pass1-$(1)*.csv run.txt \
	&& mkdir -p $(CSV) $(DATA) \
	&& $(EXE) -D $(1) -p $(DATA) -m nosync -n $(N)0 -B set -C $(CSV)/pass1- $(LOGOUT) run.txt \
	&& $(EXE) -D $(1) -p $(DATA) -c -m lazy -n $(N)0 -B get,iterate,crud -w 1 -C $(CSV)/pass1- $(LOGOUT) run.txt \
	&& mv run.txt $$@ || (echo "failed $$@" > /dev/stderr && mv run.txt $$@.failed && exit 1)

# Второй прогон:
# - просто CRUD на 10**6 с не-синхронной записью
# - потом на этой базе показываем масштабирование по CPU
#
# Эти данные попадают:
# - в cлайд №32 "NOSYNC + CRUD * 10**6"
# - в слайд №29 "Разгоняется"
$(1).pass-2.log: $(EXE)
	@ echo "$$@..." \
	&& rm -rf $(DATA) $(CSV)/pass2-$(1)*.csv $(CSV)/passR*-$(1)*.csv run.txt \
	&& mkdir -p $(CSV) $(DATA) \
	&& $(EXE) -D $(1) -p $(DATA) -m nosync -n $(N)00 -B set $(LOGOUT) run.txt \
	&& $(EXE) -D $(1) -p $(DATA) -c -m nosync -n $(N)00 -B get,iterate,crud -w 1 -C $(CSV)/pass2- $(LOGOUT) run.txt \
	&& $(EXE) -D $(1) -p $(DATA) -m nosync -n $(N)00 -B get -C $(CSV)/passR1- $(LOGOUT) run.txt \
	&& $(EXE) -D $(1) -p $(DATA) -m nosync -n $(N)00 -B get -r 2 -C $(CSV)/passR2- $(LOGOUT) run.txt \
	&& $(EXE) -D $(1) -p $(DATA) -m nosync -n $(N)00 -B get -r 4 -C $(CSV)/passR4- $(LOGOUT) run.txt \
	&& $(EXE) -D $(1) -p $(DATA) -m nosync -n $(N)00 -B get -r 8 -C $(CSV)/passR8- $(LOGOUT) run.txt \
	&& mv run.txt $$@ || (echo "failed $$@" > /dev/stderr && mv run.txt $$@.failed && exit 2)

# Третий прогон:
# - просто CRUD на 10000 с синхронной записью
#
# Эти данные попадают:
#  - в cлайд №30 "SYNC + CRUD * 10**4"
$(1).pass-3.log: $(EXE)
	@ echo "$$@..." \
	&& rm -rf $(DATA) $(CSV)/pass3-$(1)*.csv run.txt \
	&& mkdir -p $(CSV) $(DATA) \
	&& $(EXE) -D $(1) -p $(DATA) -m nosync -n $(N) -B set $(LOGOUT) run.txt \
	&& $(EXE) -D $(1) -p $(DATA) -c -m sync -n $(N) -B get,iterate,crud -w 1 -C $(CSV)/pass3- $(LOGOUT) run.txt \
	&& mv run.txt $$@ || (echo "failed $$@" > /dev/stderr && mv run.txt $$@.failed && exit 3)

endef

$(eval $(foreach 1,$(ENGINES),$(call rule)))
