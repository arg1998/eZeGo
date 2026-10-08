# Module table support (specs/code-organization.md CO-1, CO-2, CO-8).
#
# modules.cmake at the repository root calls ez_declare_module() once per module. That table is
# authoritative: a module's CMakeLists.txt lists only its sources, and its layer and dependencies
# come from here. Declaring a dependency on a module that is not listed above, or that sits in a
# higher layer, is a configure error, so the graph is acyclic and the layer rule holds by
# construction. The table also generates ez/base/modules.gen.hpp in the build tree.

set(EZ_MODULES "" CACHE INTERNAL "")

function(ez_declare_module name layer deps description)
  if(NOT name MATCHES "^[a-z][a-z0-9_]*$")
    message(FATAL_ERROR "modules.cmake: module name '${name}' must be one lower_snake_case word")
  endif()
  if(name IN_LIST EZ_MODULES)
    message(FATAL_ERROR "modules.cmake: module '${name}' is declared twice")
  endif()
  if(NOT layer MATCHES "^[0-3]$")
    message(FATAL_ERROR "modules.cmake: module '${name}' has layer '${layer}'; layers are 0 to 3")
  endif()
  foreach(d IN LISTS deps)
    if(NOT d IN_LIST EZ_MODULES)
      message(FATAL_ERROR "modules.cmake: '${name}' depends on '${d}', which is not declared above it. "
                          "Dependencies must be declared earlier in the table (this keeps the graph acyclic).")
    endif()
    if(EZ_MODULE_${d}_LAYER GREATER layer)
      message(FATAL_ERROR "modules.cmake: '${name}' (layer ${layer}) depends on '${d}' (layer ${EZ_MODULE_${d}_LAYER}). "
                          "A module may depend only on the same or a lower layer.")
    endif()
  endforeach()
  set(EZ_MODULES ${EZ_MODULES} ${name} CACHE INTERNAL "")
  set(EZ_MODULE_${name}_LAYER "${layer}" CACHE INTERNAL "")
  set(EZ_MODULE_${name}_DEPS "${deps}" CACHE INTERNAL "")
  set(EZ_MODULE_${name}_DESCRIPTION "${description}" CACHE INTERNAL "")
endfunction()

include("${PROJECT_SOURCE_DIR}/modules.cmake")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/modules.cmake")

# ---------------------------------------------------------------- generated header
set(EZ_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
set(_gen "// Generated from modules.cmake by cmake/modules.cmake. Do not edit.\n")
string(APPEND _gen "// X(name, \"name\", layer) for every first-party module, in table order.\n")
string(APPEND _gen "#pragma once\n\n#define EZ_MODULES(X)")
foreach(m IN LISTS EZ_MODULES)
  string(APPEND _gen " \\\n    X(${m}, \"${m}\", ${EZ_MODULE_${m}_LAYER})")
endforeach()
string(APPEND _gen "\n")
file(CONFIGURE OUTPUT "${EZ_GENERATED_DIR}/ez/base/modules.gen.hpp" CONTENT "${_gen}" @ONLY)
