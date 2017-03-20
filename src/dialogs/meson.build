dialogs = static_library(
  'dialogs',
  'ModalDialog.cxx',
  include_directories: inc,
  dependencies: [
    ui_dep,
  ],
)

dialogs_dep = declare_dependency(
  link_with: dialogs,
)
