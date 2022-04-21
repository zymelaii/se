cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

set(SE_HEADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../include")
set(DEVEL_PATH "${CMAKE_BINARY_DIR}/../devel")

include_directories(${SE_HEADER_PATH})

set(SE_SOURCE_FILES
	ee.c
	fnlib.c
	phead.c
	presult.c
	rout.cpp
	se-repl.cpp)

add_executable(se-repl se-repl.cpp)

target_link_libraries(se-repl PUBLIC se)

install(TARGETS se-repl RUNTIME DESTINATION "${DEVEL_PATH}/bin")