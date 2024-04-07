project('directx-mesh', 'cpp')

src = [

]

inc = include_directories('.')

dxmesh = static_library('directx-mesh', src, include_directories : inc)

dxmesh_dep = declare_dependency(
    include_directories : inc,
    link_libraries : dxmesh
)

meson.override_dependency('directx-mesh', dxmesh_dep)
