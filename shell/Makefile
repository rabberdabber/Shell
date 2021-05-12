#Name: bereket assefa
#id: 20170844

CC = gcc
CCFLAGS= -g
SOURCES = -D_BSD_SOURCE -D_GNU_SOURCE


SHELL_SRC = ish.c
SYN_SRC = ishsyn.c
LEX_SRC = ishlex.c
DYN_ARR_SRC = dynarray.c
SYN_HDR = ishsyn.h
LEX_HDR = ishlex.h
DYN_ARR_HDR = dynarray.h

build: ish

ish: $(SHELL_SRC) $(SYN_HDR) $(LEX_HDR) $(DYN_ARR_HDR)
	$(CC) $(CCFLAGS) -o $@ $(SHELL_SRC) $(SYN_SRC) $(LEX_SRC) $(DYN_ARR_SRC)


clean: 
	rm -f ish




