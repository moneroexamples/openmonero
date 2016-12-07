cmake_minimum_required(VERSION 3.6)
project(restbed_xmr)

set(CMAKE_CXX_STANDARD 14)

set(asio_DIR "ext/restbed/dependency/asio")


# include monero headers
include_directories(
        "ext/restbed/source")

add_subdirectory("ext/restbed")


set(SOURCE_FILES
        main.cpp)

add_executable(restbed_xmr
        ${SOURCE_FILES})

target_link_libraries(restbed_xmr
        restbed
        pthread)