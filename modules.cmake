# The module table (specs/code-organization.md CO-8): the single list of first-party modules.
# Order matters: a module may depend only on modules listed above it, in the same or a lower layer.
# Each name is a directory under src/ez/, a namespace (base is `ez` itself), a target ez_<name>
# with alias ez::<name>, a log category, a cvar prefix and a memory tag.
#
#                 name      layer  depends on             description
ez_declare_module(base      0      ""                     "types, platform detection, macros, assertions, strings, hashing, the module table")
ez_declare_module(cvars     0      "base"                 "runtime variables: registry, validation, startup sources, persistence, console")
ez_declare_module(log       0      "base;cvars"           "logging: per-thread rings, log thread, sinks, categories, runtime levels")
