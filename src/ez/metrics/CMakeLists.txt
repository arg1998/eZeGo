# ez::metrics — layer 0 (specs/observability.md). Today: the profiler half of the facade.
ez_module(metrics SOURCES profiler.hpp profiler.cpp)
if(EZ_PROFILER)
  # The facade header includes Tracy in profile builds, so callers need its include path.
  target_link_libraries(ez_metrics PUBLIC Tracy::TracyClient)
endif()

# Profile builds: the operator new/delete hooks, linked into executables as objects (L-6).
if(EZ_MEM_TRACE)
  add_library(ez_metrics_new_hooks OBJECT new_hooks.cpp)
  target_link_libraries(ez_metrics_new_hooks PRIVATE ez::metrics)
  ez_warnings(ez_metrics_new_hooks)
endif()
