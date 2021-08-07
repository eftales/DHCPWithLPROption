all:
	gcc dhtest.c functions.c headers.h -o test

clean:
	rm -f test *:*
