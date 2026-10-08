# First-party modules from the module table (specs/code-organization.md CO-8), in table order.
foreach(_m IN LISTS EZ_MODULES)
  add_subdirectory(ez/${_m})
endforeach()

# Executables: main() only (CO-6).
add_subdirectory(apps/ezego)
