cmake_minimum_required(VERSION 3.21.2)
cmake_policy(SET CMP0135 NEW)

project(gdatad)

set(CMAKE_CXX_STANDARD 11)

include(FetchContent)
FetchContent_Declare(
	  googletest
	  URL https://github.com/google/googletest/archive/refs/heads/main.zip
          )

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)  

include_directories(../)
include_directories(../Inc)
add_library(rfs STATIC ../Src/ring_file_system.c)
add_library(flashsim STATIC flashsim.c)

enable_testing()
add_executable(
	  runtest
	  test.cpp
	  )
target_link_libraries(
	      runtest
	      flashsim
	      rfs
	      gtest_main
	      )

include(GoogleTest)
gtest_discover_tests(runtest)
