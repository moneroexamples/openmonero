include(ExternalProject)
include(GNUInstallDirs)


ExternalProject_Add(monero
    PREFIX ${CMAKE_SOURCE_DIR}/deps
    DOWNLOAD_NO_PROGRESS 0
    URL https://github.com/monero-project/monero/archive/v0.10.3.1.tar.gz
    BUILD_IN_SOURCE 1
    BUILD_COMMAND make
    LOG_BUILD 1
    INSTALL_COMMAND ""
)
