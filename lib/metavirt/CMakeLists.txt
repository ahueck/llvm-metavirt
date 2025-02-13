set(LIB_SOURCES
  VirtCall.cpp
)

add_library(metavirt_Types STATIC
  "${LIB_SOURCES}"
)

add_library(metavirt::Types ALIAS metavirt_Types)

metavirt_target_define_file_basename(metavirt_Types)

set_property(TARGET metavirt_Types PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(metavirt_Types SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})

target_compile_definitions(metavirt_Types
        PRIVATE
        LLVM_VERSION_MAJOR=${LLVM_VERSION_MAJOR}
        METAVIRT_LOG_LEVEL=${METAVIRT_LOG_LEVEL}
        )

metavirt_target_compile_opts(metavirt_Types)

target_include_directories(
  metavirt_Types
  PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib/>
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

set_target_properties(
  metavirt_Types
  PROPERTIES
  OUTPUT_NAME "metavirtTypes"
  EXPORT_NAME "Types"
)


set(CONFIG_NAME metavirtTypes)
set(TARGETS_EXPORT_NAME ${CONFIG_NAME}Targets)

install(FILES Dimeta.h DimetaData.h DimetaIO.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

install(
  TARGETS metavirt_Types
  EXPORT ${TARGETS_EXPORT_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
  EXPORT ${TARGETS_EXPORT_NAME}
  NAMESPACE metavirt::
  DESTINATION ${METAVIRT_INSTALL_CONFIGDIR}
)

export(
  EXPORT ${TARGETS_EXPORT_NAME}
  FILE ${CMAKE_BINARY_DIR}/${TARGETS_EXPORT_NAME}.cmake
  NAMESPACE metavirt::
)

