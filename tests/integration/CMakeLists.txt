# ensure helper programs in the testing harness get built
add_subdirectory(harness)

gkfs_enable_python_testing(
    BINARY_DIRECTORIES ${CMAKE_BINARY_DIR}/src/daemon/
                       ${CMAKE_BINARY_DIR}/src/client/
                       ${CMAKE_BINARY_DIR}/tests/integration/harness/
    LIBRARY_PREFIX_DIRECTORIES ${CMAKE_PREFIX_PATH}
)

# define CTest tests for functional test groups
gkfs_add_python_test(
    NAME test_directories
    PYTHON_VERSION 3.6
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/tests/integration
    SOURCE directories/test_directories.py
)

gkfs_add_python_test(
    NAME test_shell
    PYTHON_VERSION 3.6
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/tests/integration
    SOURCE shell/
)

if(GKFS_INSTALL_TESTS)
    install(DIRECTORY harness
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/gkfs/tests/integration
        FILES_MATCHING
            REGEX ".*\\.py"
            PATTERN "__pycache__" EXCLUDE
            PATTERN ".pytest_cache" EXCLUDE
            PATTERN "gkfs.io" EXCLUDE
    )

    install(DIRECTORY directories
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/gkfs/tests/integration
        FILES_MATCHING
            REGEX ".*\\.py"
            PATTERN "__pycache__" EXCLUDE
            PATTERN ".pytest_cache" EXCLUDE
    )

    install(DIRECTORY shell
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/gkfs/tests/integration
        FILES_MATCHING
            REGEX ".*\\.py"
            PATTERN "__pycache__" EXCLUDE
            PATTERN ".pytest_cache" EXCLUDE
    )
endif()