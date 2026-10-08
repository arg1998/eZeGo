# Tests of the prototype under src/core (replaced, not renamed: naming.md §14). Exempt from the
# naming lint. Removed together with src/core.
set(EZ_LEGACY ON)

ez_test(test_core SOURCES test_core.cpp DEPS ez::core LABELS unit)
