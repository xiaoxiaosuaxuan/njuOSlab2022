NAME := $(shell basename $(PWD))
export MODULE := M2
all: $(NAME)-64.so $(NAME)-32.so
CFLAGS += -U_FORTIFY_SOURCE

include ../Makefile

.PHONY: test 

test: test.c co.h
	@gcc -g  -I. -L. -m64 test.c co.h -o test -lco-64
	@LD_LIBRARY_PATH=. ./test
