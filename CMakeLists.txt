cmake_minimum_required(VERSION 3.2)

set(PROJECT_NAME
        openmonero)

project(${PROJECT_NAME})

set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -std=c++11")

if(CMAKE_SIZEOF_VOID_P EQUAL "4")
  add_definitions(-DMDB_VL32)
endif()

if (NOT MONERO_DIR)
    set(MONERO_DIR ~/monero)
endif()

message(STATUS MONERO_DIR ": ${MONERO_DIR}")

set(MONERO_SOURCE_DIR ${MONERO_DIR}
        CACHE PATH "Path to the root directory for Monero")

# set location of monero build tree
set(MONERO_BUILD_DIR ${MONERO_SOURCE_DIR}/build/release/
        CACHE PATH "Path to the build directory for Monero")

set(MY_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/cmake"
        CACHE PATH "The path to the cmake directory of the current project")

list(APPEND CMAKE_MODULE_PATH "${MY_CMAKE_DIR}")

set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "${MONERO_BUILD_DIR}"
        CACHE PATH "Add Monero directory for library searching")

include(MyUtils)

find_package(Monero)

include_directories(/usr/include/mysql)

# include boost headers
include_directories(${Boost_INCLUDE_DIRS})

# include monero headers
include_directories("ext/restbed/source")

include_directories(
    ${MONERO_SOURCE_DIR}/build
    /usr/local/include
    /usr/local/include/mysql
    /usr/local/opt/openssl/include
)

link_directories(
    ${MONERO_BUILD_DIR}/src/crypto
    /usr/local/lib
    /usr/local/opt/openssl/lib
)

create_git_version()

configure_files(${CMAKE_CURRENT_SOURCE_DIR}/config ${CMAKE_CURRENT_BINARY_DIR}/config)

# find boost
find_package(Boost COMPONENTS
        system
        filesystem
        thread
        date_time
        chrono
        regex
        serialization
        program_options
        date_time
        REQUIRED)


# add src/ subfolder
add_subdirectory(src/)

# add ext/ subfolder
add_subdirectory(ext/)


set(SOURCE_FILES
        main.cpp)

add_executable(${PROJECT_NAME}
        ${SOURCE_FILES})

set(LIBRARIES
        myxrm
        myext
        restbed
        wallet
        blockchain_db
        cryptonote_core
        cryptonote_protocol
        cryptonote_basic
        daemonizer
        cncrypto
        blocks
        lmdb
        ringct
        common
        mnemonics
        epee
        easylogging
        mysqlpp
        mysqlclient
        readline
        easylogging
        ${Boost_LIBRARIES}
        pthread
        unbound
        curl
        dl
        cncrypto
        ssl
        crypto)

if(APPLE)
    set(LIBRARIES ${LIBRARIES} "-framework IOKit")
endif()

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND NOT WIN32)
    set(LIBRARIES ${LIBRARIES} unwind)
endif()

target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

