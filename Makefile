CC=cc
CFLAGS=-Wall -fPIC -g
INCLUDES=-I.
LFLAGS=
LIBS=
RM=rm -f

SRCS=jsa.c jsb.c jsc.c jsh.c jsini.c jsini_json.c jsini_ini.c jsx.c

OBJS=$(SRCS:.c=.o)
MAIN=jsini
JSINI=libjsini.a
TESTS=test_jsh test_decode_utf8 test_cpp test_jsx

.PHONY: depend clean

all: $(MAIN) $(TESTS)

$(MAIN): $(JSINI) main.o
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) main.o $(JSINI) $(LFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

$(JSINI): $(OBJS)
	#$(CC) -shared -o libjsini.so $(OBJS)
	ar -cvq libjsini.a $(OBJS)

test_jsh: jsh.c tests/jsh.c
	$(CC) -DTEST_JSH $(INCLUDES) $^ -o $@

test_decode_utf8: jsc.c tests/decode_utf8.c
	$(CC) $(INCLUDES) $^ -o $@

test_cpp: jsini.hpp jsini.h tests/test.cpp
  # -fno-elide-constructors
	g++ -std=c++11 -g $(INCLUDES) tests/test.cpp -o $@ libjsini.a $(LFLAGS) $(LIBS)

test_jsx: tests/test_jsx.o
	$(CC) $^ -o $@ libjsini.a $(LFLAGS) $(LIBS)

clean:
	$(RM) *.o *~ $(MAIN) $(JSINI) $(TESTS)

install:
	mkdir -p /usr/local/include/jsini
	cp *.h *.hpp /usr/local/include/jsini
	cp $(JSINI)  /usr/local/lib
