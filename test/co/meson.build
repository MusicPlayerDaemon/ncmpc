test(
  'TestCo',
  executable(
    'TestCo',
    'TestInvokeTask.cxx',
    'TestEagerInvokeTask.cxx',
    'TestEagerTask.cxx',
    'TestAwaitableHelper.cxx',
    include_directories: inc,
    dependencies: [
      gtest,
      coroutines_dep,
    ],
  ),
)
