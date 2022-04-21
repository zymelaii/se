cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(se VERSION 1.0.0 LANGUAGES C CXX)

set(SE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(SE_HEADER_PATH  ${SE_ROOT}/include)

option(SE_BUILD_EXAMPLE  "build examples"   ON)
option(SE_BUILD_UNITTEST "build unit-tests" ON)

add_subdirectory(${SE_ROOT}/src)

if (SE_BUILD_EXAMPLE)
	add_subdirectory(${SE_ROOT}/example)
endif()

if (SE_BUILD_UNITTEST)
	add_subdirectory(${SE_ROOT}/thirdparty/googletest)
	add_subdirectory(${SE_ROOT}/test)
endif()

include(GNUInstallDirs)
install(DIRECTORY "${SE_HEADER_PATH}/se" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})