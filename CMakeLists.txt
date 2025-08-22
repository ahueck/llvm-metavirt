cmake_minimum_required(VERSION 3.20)

project(
    metavirt
    VERSION 0.1.0
    DESCRIPTION "An LLVM-based plugin to resolve virtual call targets using debug metadata"
    HOMEPAGE_URL "https://github.com/ahueck/llvm-metavirt"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 20)

list(
  APPEND
  CMAKE_MODULE_PATH
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

include(metavirtToolchain)

metavirt_target_format(
  format-metavirt-sources "Formats project source files"
  TARGETS lib/metavirt/*.cpp
          lib/metavirt/*.h
          test/verifier/pass/MetavirtPass.cpp
)

add_subdirectory(lib)

if(PROJECT_IS_TOP_LEVEL)
  enable_testing()
  add_subdirectory(test)
endif()

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/metavirtConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMinorVersion
)

set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})
set(LIBRARY_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
set(BINARY_INSTALL_DIR ${CMAKE_INSTALL_BINDIR})
set(CMAKE_INSTALL_DIR ${METAVIRT_INSTALL_CONFIGDIR})

configure_package_config_file(
  ${PROJECT_SOURCE_DIR}/cmake/metavirtConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/metavirtConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_DIR}
  PATH_VARS INCLUDE_INSTALL_DIR
            LIBRARY_INSTALL_DIR
            BINARY_INSTALL_DIR
            CMAKE_INSTALL_DIR
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/metavirtConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/metavirtConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_DIR}
)

if(PROJECT_IS_TOP_LEVEL)
    feature_summary(WHAT ENABLED_FEATURES PACKAGES_FOUND PACKAGES_NOT_FOUND
      DESCRIPTION "metavirt ${PROJECT_VERSION} package and feature info:"
      INCLUDE_QUIET_PACKAGES
      QUIET_ON_EMPTY
    )
endif()
