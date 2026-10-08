# First-party modules from the module table (specs/code-organization.md CO-8), in table order.
foreach(_m IN LISTS EZ_MODULES)
  add_subdirectory(ez/${_m})
endforeach()

# The prototype: replaced, not renamed, as src/ez/ grows. Only src/application links GLFW / ImGui.
add_subdirectory(core)
add_subdirectory(application)
