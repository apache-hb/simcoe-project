args = cpp.get_supported_arguments(
    '-Wno-switch',
    '-Wno-unused-variable',
    '-Wno-unused-function',
    '/wd4189',
    '/wd4127',
    '-DD3D12MA_USING_DIRECTX_HEADERS=1'
)

libd3d12ma = library('d3d12ma', 'src/D3D12MemAlloc.cpp',
    cpp_args : args,
    include_directories : 'include',
    dependencies : [ d3d12, d3dx12, dxgi, core ]
)

d3d12ma = declare_dependency(
    link_with : libd3d12ma,
    include_directories : 'include'
)
