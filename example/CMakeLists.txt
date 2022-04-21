set(SE_SOURCE_FILES
	ee.c
	fnlib.c
	phead.c
	presult.c
	rout.cpp
	se-repl.cpp)

add_executable(se-repl se-repl.cpp)
target_link_libraries(se-repl PRIVATE se)
target_include_directories(se-repl PUBLIC ${SE_HEADER_PATH})

include(GNUInstallDirs)
install(TARGETS se-repl RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})