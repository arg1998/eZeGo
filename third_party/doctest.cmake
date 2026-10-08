# doctest: header-only test framework (specs/testing.md T-1). The target is ours (B-8).
add_library(doctest INTERFACE)
target_include_directories(doctest SYSTEM INTERFACE "${EZ_DEP_doctest_DIR}")
ez_third_party(doctest)
