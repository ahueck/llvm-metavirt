include(CMakeDependentOption)
include(CMakePackageConfigHelpers)
include(FeatureSummary)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE STRING "" FORCE)
include(FetchContent)

find_package(LLVM CONFIG HINTS "${LLVM_DIR}")
if(NOT LLVM_FOUND)
  message(STATUS "LLVM not found at: ${LLVM_DIR}.")
  find_package(LLVM REQUIRED CONFIG)
endif()

set_package_properties(LLVM PROPERTIES
  URL https://llvm.org/
  TYPE REQUIRED
  PURPOSE
  "LLVM framework installation required to compile (and apply) project metavirt."
)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}"
  PROJECT_IS_TOP_LEVEL
)

option(METAVIRT_TEST_CONFIGURE_IDE "Add targets for tests to help the IDE with completion etc." ON)
mark_as_advanced(METAVIRT_TEST_CONFIGURE_IDE)
option(METAVIRT_CONFIG_DIR_IS_SHARE "Install to \"share/cmake/\" instead of \"lib/cmake/\"" OFF)
mark_as_advanced(METAVIRT_CONFIG_DIR_IS_SHARE)

option(METAVIRT_ENABLE_COVERAGE "Enable coverage targets" OFF)
set(METAVIRT_LOG_LEVEL 0 CACHE STRING "Granularity of logger. 3 is most verbose, 0 is least.")


set(warning_guard "")
if(NOT PROJECT_IS_TOP_LEVEL)
  option(
      METAVIRT_INCLUDES_WITH_SYSTEM
      "Use SYSTEM modifier for metavirt includes to disable warnings."
      ON
  )
  mark_as_advanced(METAVIRT_INCLUDES_WITH_SYSTEM)

  if(METAVIRT_INCLUDES_WITH_SYSTEM)
    set(warning_guard SYSTEM)
  endif()
endif()

include(modules/metavirt-llvm)
include(modules/metavirt-format)
include(modules/metavirt-target-util)
if(METAVIRT_ENABLE_COVERAGE)
  include(modules/metavirt-coverage-lcov)
endif()

metavirt_find_llvm_progs(METAVIRT_CLANG_EXEC "clang-${LLVM_VERSION_MAJOR};clang" DEFAULT_EXE "clang")
metavirt_find_llvm_progs(METAVIRT_CLANGCXX_EXEC "clang++-${LLVM_VERSION_MAJOR};clang++" DEFAULT_EXE "clang++")
metavirt_find_llvm_progs(METAVIRT_LLC_EXEC "llc-${LLVM_VERSION_MAJOR};llc" DEFAULT_EXE "llc")
metavirt_find_llvm_progs(METAVIRT_OPT_EXEC "opt-${LLVM_VERSION_MAJOR};opt" DEFAULT_EXE "opt")

if(PROJECT_IS_TOP_LEVEL)
  if(NOT CMAKE_BUILD_TYPE)
    # set default build type
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
    message(STATUS "Building as debug (default)")
  endif()

  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    # set default install path
    set(CMAKE_INSTALL_PREFIX
        "${metavirt_SOURCE_DIR}/install/metavirt"
        CACHE PATH "Default install path" FORCE
    )
    message(STATUS "Installing to (default): ${CMAKE_INSTALL_PREFIX}")
  endif()

    # METAVIRT_DEBUG_POSTFIX is only used for Config
    if(CMAKE_DEBUG_POSTFIX)
      set(METAVIRT_DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
    else()
      set(METAVIRT_DEBUG_POSTFIX "-d")
    endif()

  if(NOT CMAKE_DEBUG_POSTFIX AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_DEBUG_POSTFIX ${METAVIRT_DEBUG_POSTFIX})
  endif()
else()
  set(METAVIRT_DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
endif()

include(GNUInstallDirs)

set(METAVIRT_PREFIX ${PROJECT_NAME})
set(TARGETS_EXPORT_NAME ${METAVIRT_PREFIX}Targets)

if(METAVIRT_CONFIG_DIR_IS_SHARE)
  set(METAVIRT_INSTALL_CONFIGDIR ${CMAKE_INSTALL_DATAROOTDIR}/cmake/${PROJECT_NAME})
else()
  set(METAVIRT_INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
endif()
