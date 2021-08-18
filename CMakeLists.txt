project(wit)
cmake_minimum_required(VERSION 2.8.7)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall")
set(CMAKE_CXX_FLAGS_REL_WITH_ASSERT "-O3")

function(add_test_wit name file)
    add_executable(${name} ${file})
    add_custom_command(
        TARGET ${name}
        COMMENT "Run test: ${name}"
        POST_BUILD
        COMMAND ${name}
        )
endfunction()

include_directories(include)
add_subdirectory(test)


