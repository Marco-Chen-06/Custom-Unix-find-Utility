CC = gcc
CFLAGS += -g

simplefind : simplefind.c
	${CC} ${CFLAGS} -o $@ $^

.PHONY: clean

clean:
	@rm -rvf *.o

