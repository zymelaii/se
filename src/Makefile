CC := gcc -I../include -c

all: clean release debug

release:
	${CC} -DNDEBUG alloc.c
	${CC} -DNDEBUG token.c
	${CC} -DNDEBUG priority.c
	${CC} -DNDEBUG stack.c
	${CC} -DNDEBUG type.c
	${CC} -DNDEBUG type/object.c
	${CC} -DNDEBUG type/number.c
	${CC} -DNDEBUG type/function.c
	${CC} -DNDEBUG type/array.c
	${CC} -DNDEBUG ref.c
	${CC} -DNDEBUG exception.c
	${CC} -DNDEBUG parser.c
	${CC} -DNDEBUG context.c
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
	${CC} -DDEBUG ref.c
	${CC} -DDEBUG parser.c
	${CC} -DDEBUG context.c
	ar -rcs libse_d.a *.o

clean:
	-@rm -rf *.a *.o *.exe *.gch