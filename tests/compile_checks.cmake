file(GLOB_RECURSE public_headers CONFIGURE_DEPENDS RELATIVE
  "${PROJECT_SOURCE_DIR}/src" "${PROJECT_SOURCE_DIR}/src/*.hpp"
  "${PROJECT_SOURCE_DIR}/src/*.h")
set(header_sources)
foreach(header IN LISTS public_headers)
  string(MAKE_C_IDENTIFIER "${header}" identifier)
  set(source "${CMAKE_CURRENT_BINARY_DIR}/${identifier}.cpp")
  file(WRITE "${source}" "#include <${header}>\n")
  list(APPEND header_sources "${source}")
endforeach()

add_library(grevir_stepper_compile OBJECT native_compile.cpp ${header_sources})
target_link_libraries(grevir_stepper_compile PRIVATE grevir::stepper)
set_target_properties(grevir_stepper_compile PROPERTIES CXX_EXTENSIONS OFF)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "^(AppleClang|Clang|GNU|MSVC)$")
  message(FATAL_ERROR "Stepper pin-claim probes require a supported C++ compiler driver")
endif()
add_custom_target(grevir_stepper_claim_checks ALL
  COMMAND "${CMAKE_COMMAND}"
    "-DCXX=${CMAKE_CXX_COMPILER}"
      "-DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}"
    "-DINCLUDE_DIRS=$<TARGET_PROPERTY:grevir_stepper_compile,INCLUDE_DIRECTORIES>"
    "-DCASE_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/pin_claim_probe.cpp"
    "-DLOG_DIR=${CMAKE_CURRENT_BINARY_DIR}/claim-results"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/check_pin_claims.cmake"
  COMMENT "Checking stepper pin claims"
  VERBATIM)
