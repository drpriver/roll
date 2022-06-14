cmake_minimum_required(VERSION 3.14) # This is a guess
project(Roll VERSION 1.0.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

if(WIN32)
add_compile_options("-D_CRT_NONSTDC_NO_DEPRECATE" "-D_CRT_SECURE_NO_WARNINGS")
elseif(APPLE)
elseif(UNIX)
set(LIBM_LIBRARIES m)
endif()

add_executable(roll roll/roll.c)
target_link_libraries(roll ${LIBM_LIBRARIES})
install(TARGETS roll DESTINATION bin)
