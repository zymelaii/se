CC := gcc -I../include -c

all: clean
	${CC} alloc.c
	${CC} token.c
	${CC} priority.c
	${CC} stack.c
	${CC} type.c
	${CC} type/object.c
	${CC} type/number.c
	${CC} type/function.c
	${CC} type/array.c
	${CC} parser.c
	ar -rcs libse.a *.o
clean:
	-@rm -rf *.a *.o *.exe *.gch