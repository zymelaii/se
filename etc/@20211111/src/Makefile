CC := gcc -I../include -c

all: clean release debug

release:
	${CC} alloc.c
	${CC} token.c
	${CC} priority.c
	${CC} stack.c
	${CC} type.c
	${CC} type/object.c
	${CC} type/number.c
	${CC} type/function.c
	${CC} type/array.c
	${CC} exception.c
	${CC} parser.c
	ar -rcs libse.a *.o

debug:
	${CC} -DDEBUG alloc.c
	${CC} -DDEBUG token.c
	${CC} -DDEBUG priority.c
	${CC} -DDEBUG stack.c
	${CC} -DDEBUG type.c
	${CC} -DDEBUG type/object.c
	${CC} -DDEBUG type/number.c
	${CC} -DDEBUG type/function.c
	${CC} -DDEBUG type/array.c
	${CC} -DDEBUG exception.c
	${CC} -DDEBUG parser.c
	ar -rcs libse_d.a *.o

clean:
	-@rm -rf *.a *.o *.exe *.gch