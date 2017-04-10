cmake_minimum_required(VERSION 3.2)

project(myxrm)

set(SOURCE_FILES
		MicroCore.cpp
		tools.cpp
		CmdLineOptions.cpp
		CurrentBlockchainStatus.cpp
		MySqlConnector.cpp
		MySqlAccounts.cpp
		ssqlses.cpp
		YourMoneroRequests.cpp
		TxSearch.cpp
		rpccalls.cpp
		OutputInputIdentification.cpp
		version.h.in)

# make static library called libmyxrm
# that we are going to link to
# in the root CMakeLists.txt file
add_library(myxrm
		STATIC
		${SOURCE_FILES})
