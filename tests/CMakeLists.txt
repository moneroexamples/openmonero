macro(add_om_test _TEST_NAME)

    add_executable(${_TEST_NAME}_tests
            "${_TEST_NAME}_tests.cpp"
            JsonTx.cpp)

    target_link_libraries(${_TEST_NAME}_tests
        PRIVATE
          gtest gtest_main
          gmock gmock_main
          myxmr)

    add_test(NAME ${_TEST_NAME}_tests COMMAND ${_TEST_NAME}_tests)

endmacro()


resource_dir("res")

add_om_test(mysql)
add_om_test(microcore)
add_om_test(bcstatus)
add_om_test(txsearch)


######################################
########## CODE COVERAGE #############
######################################

#include(CodeCoverage)

#APPEND_COVERAGE_COMPILER_FLAGS()

#set(COVERAGE_LCOV_EXCLUDES
    #'*/ext/*' '*boost/*' '*c++/*'
     #'*/monero/*' '*googletest/*' '*googlemock/*'
     #'*restbed/*' '*/mysql++/*' '*tests/*'
     #'*src/xmregcore/*')

 #SETUP_TARGET_FOR_COVERAGE_LCOV(
       #NAME coverage
       #EXECUTABLE mysql_tests
                  #microcore_tests
                  #bcstatus
                  #txsearch)

