src = [ 'imfilebrowser.cpp' ]
inc = include_directories('.')

libfilebrowser = library('imfilebrowser', src,
    include_directories : inc,
    dependencies : imgui
)

filebrowser = declare_dependency(
    link_with : libfilebrowser,
    include_directories : inc,
)
