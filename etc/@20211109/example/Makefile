CFLAGS        := -I. -I../include -L. -L../src -L../lib -std=c++1z -O2
CFLAGS_STATIC := -Wl,-Bstatic -lstdc++ -lgcc_eh -lpthread -Wl,-Bdynamic
CLIBS         := -lse

all: clean rebuild tokenizer parser evaluator

rebuild:
	-@echo "[ RUN      ] build se library"
	-@cd ../src && make -s
	-@echo "[       OK ] libse.a"

tokenizer-static:
	-@g++ ${CFLAGS} -o tokenizer tokenizer_v1.1.0.cpp ${CLIBS} ${CFLAGS_STATIC}

tokenizer:
	-@echo "[ RUN      ] build tokenizer v1.1.0"
	-@g++ ${CFLAGS} -o tokenizer tokenizer_v1.1.0.cpp ${CLIBS}
	-@echo "[       OK ] tokenizer"

parser-static:
	-@g++ ${CFLAGS} -o parser parser_v1.2.0.cpp ${CLIBS} ${CFLAGS_STATIC}

parser:
	-@echo "[ RUN      ] build parser v1.2.0"
	-@g++ ${CFLAGS} -o parser parser_v1.2.0.cpp ${CLIBS}
	-@echo "[       OK ] parser"

evaluator-static:
	-@g++ ${CFLAGS} -o evaluator evaluator_v1.0.0.cpp ${CLIBS} ${CFLAGS_STATIC}

evaluator:
	-@echo "[ RUN      ] build evaluator v1.0.0"
	-@g++ ${CFLAGS} -g -O0 -o evaluator evaluator_v1.0.0.cpp ${CLIBS}
	-@echo "[       OK ] evaluator"

clean:
	-@rm -rf *.exe