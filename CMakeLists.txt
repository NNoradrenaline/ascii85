cmake_minimum_required(VERSION 3.16)
project(ascii85 VERSION 2.2.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(ascii85 ascii85.c)

if(MSVC)
  target_compile_options(ascii85 PRIVATE /W4 /permissive-)
else()
  target_compile_options(ascii85 PRIVATE -Wall -Wextra -Wconversion -Wshadow -pedantic)
endif()

include(GNUInstallDirs)
install(TARGETS ascii85 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(NOT WIN32)
  install(FILES ascii85.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endif()

include(CTest)
if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_test(NAME ascii85-tests
           COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests.py $<TARGET_FILE:ascii85>)
endif()
