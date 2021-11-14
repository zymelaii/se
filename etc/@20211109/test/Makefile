CFLAGS := -I. -I../include -L. -L../src -L../lib -std=c++1z -O2
CLIBS  := -lgtest_main -lgtest -lse

all: clean rebuild gtest_token gtest_type
	-@echo ""
	-@echo "[----------]"
	-@echo "[==========]"
	-@echo "[ ALL DONE ]"

rebuild:
	-@echo "[ RUN      ] rebuild archieve"
	-@cd ../src && make -s
	-@echo "[       OK ] archieve (libse.a)"

gtest_token:
	-@echo "[ RUN      ] build tokenTest"
	-@g++ ${CFLAGS} -o gtest_token gtest_token.cc ${CLIBS}
	-@echo "[       OK ] tokenTest"

gtest_type:
	-@echo "[ RUN      ] build typeTest"
	-@g++ ${CFLAGS} -o gtest_type gtest_type.cc ${CLIBS}
	-@echo "[       OK ] typeTest"

clean:
	-@rm -rf *.exe