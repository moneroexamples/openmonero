cmake_minimum_required(VERSION 3.10)

project(myxmr)

set(SOURCE_FILES
		CmdLineOptions.cpp
		CurrentBlockchainStatus.cpp
        db/MySqlConnector.cpp
        db/MySqlAccounts.cpp
        db/ssqlses.cpp
		OpenMoneroRequests.cpp
		TxSearch.cpp
        RPCCalls.cpp
		omversion.h.in
		BlockchainSetup.cpp
		ThreadRAII.cpp
        db/MysqlPing.cpp
        TxUnlockChecker.cpp
        utils.cpp
        RandomOutputs.cpp)

add_library(myxmr STATIC
    ${SOURCE_FILES})

target_link_libraries(myxmr 
    PUBLIC
    XMREG::core 
    ${restbed_LIBRARY}
    ${MYSQLPP_LIBRARIES})

target_include_directories(myxmr 
    PUBLIC
    ${restbed_INCLUDE}
    ${MYSQLPP_INCLUDE_DIRS})
