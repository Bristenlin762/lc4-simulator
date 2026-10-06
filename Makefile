CC = cc
CPPFLAGS = -I.
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS =

.PHONY: all clean test demo
all: lc4_debugger

lc4_debugger: lc4_debugger.o lc4_cpu.o lc4_hex.o
	$(CC) $(LDFLAGS) $^ -o $@

lc4_debugger.o: lc4_debugger.c lc4_cpu.h lc4_hex.h
lc4_cpu.o: lc4_cpu.c lc4_cpu.h
lc4_hex.o: lc4_hex.c lc4_hex.h lc4_cpu.h

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

tests/test_cpu: tests/test_cpu.c lc4_cpu.o lc4_hex.o lc4_cpu.h lc4_hex.h
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_cpu.c lc4_cpu.o lc4_hex.o $(LDFLAGS) -o $@

test: lc4_debugger tests/test_cpu
	./tests/test_cpu
	python3 tests/test_cli.py

demo: lc4_debugger
	./lc4_debugger examples/sum.hex

clean:
	rm -f lc4_debugger *.o tests/test_cpu
