CC = gcc
DEBUG = -g3 -O0
CFLAGS = -Wall -Wshadow -Wunreachable-code -Wredundant-decls -Wmissing-declarations \
	 -Wold-style-definition -Wmissing-prototypes -Wdeclaration-after-statement \
	 -Wextra -Werror -Wpedantic -Wno-return-local-addr -Wunsafe-loop-optimizations \
	 -Wuninitialized $(WERROR) $(DEBUG)

H2F = hex-2-float
F2H = float-2-hex
PROGS = $(H2F) $(F2H)
TAR_FILE = ${LOGNAME}_Lab03.tar.gz

all: $(PROGS)

$(H2F): $(H2F).o
	$(CC) -o $(H2F) $(H2F).o -lm

$(H2F).o: $(H2F).c
	$(CC) $(CFLAGS) -c $(H2F).c

$(F2H): $(F2H).o
	$(CC) -o $(F2H) $(F2H).o

$(F2H).o: $(F2H).c
	$(CC) $(CFLAGS) -c $(F2H).c

clean cls:
	rm -f $(PROGS) *.o *~ \

tar: clean
	rm -f $(TAR_FILE)
	tar cvfa $(TAR_FILE) *.[ch] [Mm]akefile

git lazy:
	if [ ! -d .git ] ; then git init; fi
	git add *.[ch] ?akefile
	git commit -m "don't mess it up again, my dude"
