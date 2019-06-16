cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME
        openmonero)

project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Debug)

if(CMAKE_SIZEOF_VOID_P EQUAL "4")
  add_definitions(-DMDB_VL32)
endif()

option(BUILD_TESTS "Build tests for the project" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/src/xmregcore/cmake")

include(MyUtils)

find_package(MySQL++ REQUIRED)
find_package(Restbed REQUIRED)


create_git_version()
configure_files(${CMAKE_CURRENT_SOURCE_DIR}/config 
                ${CMAKE_CURRENT_BINARY_DIR}/config)

# add XMREGCORE submodule
set(BUILD_XMREGCORE_TESTS OFF CACHE INTERNAL "")
add_subdirectory(src/xmregcore)    

# add src/ subfolder
add_subdirectory(src/)

# add ext/ subfolder
add_subdirectory(ext/)

add_executable(${PROJECT_NAME}
    main.cpp)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE  myxmr)

configure_files(${CMAKE_CURRENT_SOURCE_DIR}/sql 
                ${CMAKE_CURRENT_BINARY_DIR}/sql)

            
if (${BUILD_TESTS})
    enable_testing()
    add_subdirectory(src/xmregcore/ext/googletest)
    add_subdirectory(tests)
endif()
