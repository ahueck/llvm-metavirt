set(PASS_SOURCES
  MetavirtPass.cpp
)

metavirt_llvm_module(metavirt_TransformPass
  "${PASS_SOURCES}"
  INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/lib/type
)
mark_as_advanced(LLVM_METAVIRT_TRANSFORMPASS_LINK_INTO_TOOLS)

set_target_properties(
  metavirt_TransformPass
  PROPERTIES
  OUTPUT_NAME "${PROJECT_NAME}TransformPass"
  EXPORT_NAME "TransformPass"
)

add_library(metavirt::TransformPass ALIAS metavirt_TransformPass)

target_compile_features(metavirt_TransformPass PUBLIC cxx_std_17)

target_include_directories(metavirt_TransformPass ${warning_guard}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
          ${PROJECT_SOURCE_DIR}/lib/
)

