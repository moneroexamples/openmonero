

macro(add_om_test _TEST_NAME)

    add_executable(${_TEST_NAME}_tests
            ${_TEST_NAME}_tests.cpp
            #${MONERO_DIR}/tests/core_tests/chaingen.cpp
            #${MONERO_DIR}/tests/core_tests/chaingen001.cpp
            )

    target_link_libraries(${_TEST_NAME}_tests
            gtest gtest_main
            ${LIBRARIES})

    add_test(NAME ${_TEST_NAME}_tests COMMAND ${_TEST_NAME}_tests)

endmacro()

resource_dir("./res")

add_om_test(mysql)
add_om_test(microcore)

SETUP_TARGET_FOR_COVERAGE(
        NAME mysql_coverage                    # New target name
        EXECUTABLE mysql_tests)


SETUP_TARGET_FOR_COVERAGE(
        NAME microcore_coverage                    # New target name
        EXECUTABLE microcore_tests)