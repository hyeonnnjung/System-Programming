testlib : main.o bitmap.o debug.o hash.o hex_dump.o list.o
	gcc -o testlib main.o bitmap.o debug.o hash.o hex_dump.o list.o
main.o : limits.h round.h main.c
	gcc -c -o main.o main.c
bitmap.o : limits.h round.h bitmap.h bitmap.c
	gcc -c -o bitmap.o bitmap.c
debug.o : limits.h round.h debug.h debug.c
	gcc -c -o debug.o debug.c
hash.o : limits.h round.h hash.h hash.c
	gcc -c -o hash.o hash.c
hex_dump.o : limits.h round.h hex_dump.h hex_dump.c
	gcc -c -o hex_dump.o hex_dump.c
list.o : limits.h round.h list.h list.c
	gcc -c -o list.o list.c


clean:
	rm -f testlib main.o bitmap.o debug.o hash.o hex_dump.o list.o