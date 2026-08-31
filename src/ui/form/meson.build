form = static_library(
  'form',
  'FormRow.cxx',
  'StringEditRow.cxx',
  include_directories: inc,
  dependencies: [
    ui_dep,
    dialogs_dep,
    util_dep,
  ],
)

form_dep = declare_dependency(
  link_with: form,
)
