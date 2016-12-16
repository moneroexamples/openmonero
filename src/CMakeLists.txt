cmake_minimum_required(VERSION 3.6)

project(myxrm)

set(SOURCE_FILES
		MicroCore.cpp
		tools.cpp
		CmdLineOptions.cpp MySqlConnector.h TxSearch.h CurrentBlockchainStatus.h ssqlses.h MySqlAccounts.h)

# make static library called libmyxrm
# that we are going to link to
# in the root CMakeLists.txt file
add_library(myxrm
		STATIC
		${SOURCE_FILES})
