cmake_minimum_required(VERSION 3.2)

set(PROJECT_NAME
        openmonero)

project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Debug)

if(CMAKE_SIZEOF_VOID_P EQUAL "4")
  add_definitions(-DMDB_VL32)
endif()

if (NOT MONERO_DIR)
    set(MONERO_DIR ~/monero)
endif()

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

option(BUILD_TESTS "Build tests for the project" OFF)

message(STATUS MONERO_DIR ": ${MONERO_DIR}")

set(MONERO_SOURCE_DIR ${MONERO_DIR}
        CACHE PATH "Path to the root directory for Monero")

# set location of monero build tree
set(MONERO_BUILD_DIR ${MONERO_SOURCE_DIR}/build/release/
        CACHE PATH "Path to the build directory for Monero")

set(MY_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/cmake"
        CACHE PATH "The path to the cmake directory of the current project")

list(APPEND CMAKE_MODULE_PATH "${MY_CMAKE_DIR}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/src/xmregcore/cmake")

set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "${MONERO_BUILD_DIR}"
        CACHE PATH "Add Monero directory for library searching")

include(MyUtils)


if (${BUILD_TESTS})
    include(CodeCoverage)
    APPEND_COVERAGE_COMPILER_FLAGS()
    set(COVERAGE_EXCLUDES
            '${CMAKE_SOURCE_DIR}/ext/*'
            'asio/*'
            '${MONERO_SOURCE_DIR}/*'
            '${CMAKE_SOURCE_DIR}/tests/*')
endif()

find_package(MYSQL)

if (NOT MYSQL_INCLUDE_DIR)
    message(SEND_ERROR "MySQL libraries not found! 
            Please install mysql++/mysqlpp libraries")
    return()
endif()




include_directories(${MYSQL_INCLUDE_DIR})

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

configure_files(${CMAKE_CURRENT_SOURCE_DIR}/config 
                ${CMAKE_CURRENT_BINARY_DIR}/config)


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


# add XMREGCORE submodule
set(BUILD_XMREGCORE_TESTS OFF CACHE INTERNAL "")
add_subdirectory(src/xmregcore)    


# add src/ subfolder
add_subdirectory(src/)

# add ext/ subfolder
add_subdirectory(ext/)


set(SOURCE_FILES
        main.cpp)

add_executable(${PROJECT_NAME}
        ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME}
        PRIVATE src/xmregcore)

# include monero headers
target_include_monero_directories(${PROJECT_NAME})    

set(LIBRARIES
        myxrm
        myxrmcore
        ${Monero_LIBRARIES}
        restbed
        mysqlpp
        mysqlclient
        ${Boost_LIBRARIES}
        sodium
        pthread
        unbound
        curl
        ssl
        crypto)

if(APPLE)
    set(LIBRARIES ${LIBRARIES} "-framework IOKit -framework Foundation")
else()
    set(LIBRARIES ${LIBRARIES} atomic)
endif()

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND NOT WIN32)
    set(LIBRARIES ${LIBRARIES} unwind)
endif()


if (WIN32)
    set(LIBRARIES ${LIBRARIES}
            wsock32
            ntdll
            ws2_32
            Iphlpapi)
else()
    set(LIBRARIES ${LIBRARIES} dl)
endif()

find_package(HIDAPI)
set(LIBRARIES ${LIBRARIES} ${HIDAPI_LIBRARIES})

target_link_libraries(${PROJECT_NAME} ${LIBRARIES})


if (${BUILD_TESTS})
    include_directories(
        ${CMAKE_SOURCE_DIR}/ext/googletest/googletest/include
        ${CMAKE_SOURCE_DIR}/ext/googletest/googlemock/include)
endif()

configure_files(${CMAKE_CURRENT_SOURCE_DIR}/sql ${CMAKE_CURRENT_BINARY_DIR}/sql)

if (${BUILD_TESTS})
    enable_testing()
    add_subdirectory(ext/googletest)
    add_subdirectory(tests)
endif()
