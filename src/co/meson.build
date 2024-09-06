coroutines_compile_args = []

if compiler.get_id() == 'gcc'
  coroutines_compile_args += '-fcoroutines'
elif compiler.get_id() == 'clang' and compiler.version().version_compare('<15')
  coroutines_compile_args += '-fcoroutines-ts'
endif

coroutines_dep = declare_dependency(
  compile_args: coroutines_compile_args,
)
