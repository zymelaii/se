set(SE_UNITTEST_BINS
	token_test
	type_test)

set(GTEST_LIBS
	gtest
	gtest_main)

set(SE_LIBS
	se)

set(SE_UNITTEST_LIB_DEPS ${GTEST_LIBS} ${SE_LIBS})

add_executable(token_test gtest_token.cc)
target_link_libraries(token_test PUBLIC ${SE_UNITTEST_LIB_DEPS})

add_executable(type_test gtest_type.cc)
target_link_libraries(type_test PUBLIC ${SE_UNITTEST_LIB_DEPS})

include(GNUInstallDirs)
install(TARGETS ${SE_UNITTEST_BINS} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
