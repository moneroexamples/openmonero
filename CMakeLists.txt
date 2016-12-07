cmake_minimum_required(VERSION 3.6)
project(restbed_xmr)

set(CMAKE_CXX_STANDARD 14)

# include monero headers
include_directories(
        "ext/restbed/source")

# add ext/ subfolder
add_subdirectory(ext/)

set(SOURCE_FILES
        main.cpp)

add_executable(restbed_xmr
        ${SOURCE_FILES})

target_link_libraries(restbed_xmr
        myext
        restbed
        pthread)