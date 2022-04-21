set(SE_SOURCE_FILES
	type/array.c
	type/function.c
	type/number.c
	type/object.c
	type.c
	ref.c
	exception.c
	priority.c
	token.c
	alloc.c
	stack.c
	parser.c
	context.c)

add_library(se STATIC ${SE_SOURCE_FILES})
target_include_directories(se PUBLIC ${SE_HEADER_PATH})

include(GNUInstallDirs)
install(TARGETS se ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})