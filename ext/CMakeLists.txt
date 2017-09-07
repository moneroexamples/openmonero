cmake_minimum_required(VERSION 3.2)

# now build myext library from other files
project(myext)

# first build mstch template library
add_subdirectory("restbed")

set(SOURCE_FILES
        format.cc)

# make static library called libmyxrm
# that we are going to link to
# in the root CMakeLists.txt file
add_library(myext
        STATIC
        ${SOURCE_FILES})




