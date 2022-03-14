CC = gcc
CFLAGS = -Wall -Wextra -Wunused -O2 -Werror -Wpointer-arith -Wpedantic -g -std=gnu99

# to add tracefiles, add filenames or other macros separated by commas,
# e.g. BASE_TRACEFILES,COALESCE_TRACEFILES,my_test_trace.rep
# TRACEFILES = BASE_TRACEFILES,COALESCE_TRACEFILES,REALLOC_TRACEFILES
TRACEFILES = BASE_TRACEFILES


OBJS = mdriver.o memlib.o fsecs.o fcyc.o clock.o ftimer.o
EXECS = mdriver inline_tests

all: $(EXECS)

mdriver : mdriver% : $(OBJS) mm%.o
	$(CC) $(CFLAGS) $^ -o $@

inline_tests: mminline-tests.c
	$(CC) $(CFLAGS) $^ -o $@

mdriver.o: mdriver.c fsecs.h fcyc.h clock.h memlib.h config.h mm.h
	$(CC) $(CFLAGS) -D DEFAULT_TRACEFILES=$(TRACEFILES) -c mdriver.c

memlib.o: memlib.c memlib.h
fsecs.o: fsecs.c fsecs.h config.h
fcyc.o: fcyc.c fcyc.h
ftimer.o: ftimer.c ftimer.h config.h
clock.o: clock.c clock.h
test.o: mminline-tests.c 

mm.o: mm.c mm.h memlib.h mminline.h

clean:
	rm -f *~ *.o $(EXECS)
