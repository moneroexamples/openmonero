cmake_minimum_required(VERSION 3.2)
project(restbed_xmr)

set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -std=c++14")

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


# set location of monero static libraries
set(MONERO_LIBS_DIR
        /opt/monero-dev/libs)

# set location of moneroheaders
set(MONERO_HEADERS_DIR
        /opt/monero-dev/headers)

# include monero headers
include_directories(
        ${MONERO_HEADERS_DIR}/src
        ${MONERO_HEADERS_DIR}/external
        ${MONERO_HEADERS_DIR}/contrib/epee/include
        ${MONERO_HEADERS_DIR}/external/db_drivers/liblmdb)

include_directories(/usr/include/mysql)

# get individual monero static libraries
# that are needed in this project

add_library(common STATIC IMPORTED)
set_property(TARGET common PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/libcommon.a)

add_library(blocks STATIC IMPORTED)
set_property(TARGET blocks PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/libblocks.a)

add_library(cryptoxmr STATIC IMPORTED)
set_property(TARGET cryptoxmr
        PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/libcrypto.a)

add_library(cryptonote_core STATIC IMPORTED)
set_property(TARGET cryptonote_core
        PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/libcryptonote_core.a)

add_library(cryptonote_protocol STATIC IMPORTED)
set_property(TARGET cryptonote_protocol
        PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/libcryptonote_protocol.a)

add_library(mnemonics STATIC IMPORTED)
set_property(TARGET mnemonics
        PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/libmnemonics.a)

add_library(blockchain_db STATIC IMPORTED)
set_property(TARGET blockchain_db
        PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/libblockchain_db.a)

add_library(lmdb STATIC IMPORTED)
set_property(TARGET lmdb
        PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/liblmdb.a)

add_library(ringct STATIC IMPORTED)
set_property(TARGET ringct
        PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/libringct.a)

add_library(wallet STATIC IMPORTED)
set_property(TARGET wallet
        PROPERTY IMPORTED_LOCATION ${MONERO_LIBS_DIR}/libwallet.a)

# include boost headers
include_directories(${Boost_INCLUDE_DIRS})

# include monero headers
include_directories(
        "ext/restbed/source")

# add src/ subfolder
add_subdirectory(src/)

# add ext/ subfolder
add_subdirectory(ext/)

set(SOURCE_FILES
        main.cpp)

add_executable(restbed_xmr
        ${SOURCE_FILES})


target_link_libraries(restbed_xmr
        myxrm
        myext
        restbed
        wallet
        cryptonote_core
        cryptonote_protocol
        blockchain_db
        cryptoxmr
        blocks
        lmdb
        ringct
        common
        mnemonics
        mysqlpp
        mysqlclient
        ${Boost_LIBRARIES}
        pthread
        unbound
        unwind
        curl
        dl
        crypto)