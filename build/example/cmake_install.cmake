# Install script for directory: C:/Users/melaii/Desktop/se/example

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Users/melaii/Desktop/se/build")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "D:/EnvrSupport/gcc-11.2.0-mingw-w64ucrt-9.0.0-r5/bin/objdump.exe")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "C:/Users/melaii/Desktop/se/build/../devel/bin/se-repl.exe")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "C:/Users/melaii/Desktop/se/build/../devel/bin" TYPE EXECUTABLE FILES "C:/Users/melaii/Desktop/se/build/example/se-repl.exe")
  if(EXISTS "$ENV{DESTDIR}/C:/Users/melaii/Desktop/se/build/../devel/bin/se-repl.exe" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/C:/Users/melaii/Desktop/se/build/../devel/bin/se-repl.exe")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "D:/EnvrSupport/gcc-11.2.0-mingw-w64ucrt-9.0.0-r5/bin/strip.exe" "$ENV{DESTDIR}/C:/Users/melaii/Desktop/se/build/../devel/bin/se-repl.exe")
    endif()
  endif()
endif()

