CC = gcc
CFLAGS = -g -Wall -Wvla -std=c99 -pthread -D_XOPEN_SOURCE=700
LDFLAGS = -pthread -Wall
EXE = srv clt
TEST = list/test_list

all: $(EXE)

srv: serveur.o list/list.o user.o utils.o
	$(CC) $(LDFLAGS) $^ -o $@

clt: client.o buffer/buffer.o utils.o
	$(CC) $(LDFLAGS) $^ -o $@

client.o: client.c
serveur.o: serveur.c list/list.h
user.o: user.c user.h
utils.o: utils.c utils.h

list/list.o: list/list.c list/list.h
list/test_list.o: list/test_list.c list/list.h
list/test_list: list/test_list.o list/list.o
	$(CC) $(LDFLAGS) $^ -o $@

buffer/buffer.o: buffer/buffer.c buffer/buffer.h

test: testlist

testlist: list/test_list
	valgrind --leak-check=full $^

clean:
	rm -f *.o list/*.o buffer/*.o $(EXE) $(TEST)