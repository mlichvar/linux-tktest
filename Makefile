KDIR ?= $(shell pwd)/linux
MYCFLAGS=-O2 -Wall -g

all: tk_test

tk_test: main.o missing.o regress.o timekeeping.o tk_test.o
	$(CC) $(MYCFLAGS) -o $@ $^ -lm

missing.o: missing.c
	$(MAKE) -C $(KDIR) CC=$$PWD/gcc-filter M=$$PWD $@

timekeeping.c: $(KDIR)/kernel/time/timekeeping.c
	ln -s $^ $@

timekeeping.o: timekeeping.c
	$(MAKE) -C $(KDIR) CC=$$PWD/gcc-filter M=$$PWD CFLAGS_timekeeping.o=-I$(KDIR)/kernel/time $@

tk_test.o: tk_test.c
	$(MAKE) -C $(KDIR) CC=$$PWD/gcc-filter M=$$PWD $@

main.o: main.c
	$(CC) $(MYCFLAGS) -c $^

regress.o: regress.c
	$(CC) $(MYCFLAGS) -c $^

clean:
	rm -f *.o .*.cmd tk_test timekeeping.c
