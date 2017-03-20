dialogs = static_library(
  'dialogs',
  'ModalDialog.cxx',
  'YesNoDialog.cxx',
  include_directories: inc,
  dependencies: [
    ui_dep,
    intl_dep,
  ],
)

dialogs_dep = declare_dependency(
  link_with: dialogs,
)
