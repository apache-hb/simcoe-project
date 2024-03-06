libdds_texture_loader = library('DDSTextureLoader', 'src/DDSTextureLoader12.cpp',
    include_directories : 'include',
    cpp_args : [ '-DUSING_DIRECTX_HEADERS=1' ],
    dependencies : d3dx12
)

dds_texture_loader = declare_dependency(
    include_directories : include_directories('include'),
    link_with : libdds_texture_loader,
    dependencies : d3dx12,
    compile_args : [ '-DUSING_DIRECTX_HEADERS=1' ],
)
