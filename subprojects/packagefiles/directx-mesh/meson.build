project('directx-mesh', 'cpp',
    license : 'MIT',
    license_files : [ 'LICENSE' ]
)

src = [
    'DirectXMesh/DirectXMeshAdjacency.cpp',
    'DirectXMesh/DirectXMeshClean.cpp',
    'DirectXMesh/DirectXMeshConcat.cpp',
    'DirectXMesh/DirectXMeshGSAdjacency.cpp',
    'DirectXMesh/DirectXMeshletGenerator.cpp',
    'DirectXMesh/DirectXMeshNormals.cpp',
    'DirectXMesh/DirectXMeshOptimize.cpp',
    'DirectXMesh/DirectXMeshOptimizeLRU.cpp',
    'DirectXMesh/DirectXMeshOptimizeTVC.cpp',
    'DirectXMesh/DirectXMeshRemap.cpp',
    'DirectXMesh/DirectXMeshTangentFrame.cpp',
    'DirectXMesh/DirectXMeshUtil.cpp',
    'DirectXMesh/DirectXMeshValidate.cpp',
    'DirectXMesh/DirectXMeshVBReader.cpp',
    'DirectXMesh/DirectXMeshVBWriter.cpp',
    'DirectXMesh/DirectXMeshWeldVertices.cpp',
]

inc = include_directories('.')

dxmesh = static_library('directx-mesh', src,
    include_directories : inc,
    override_options : [ 'unity=off' ]
)

dxmesh_dep = declare_dependency(
    include_directories : inc,
    link_with : dxmesh
)

meson.override_dependency('directx-mesh', dxmesh_dep)

install_data(meson.project_license_files(),
    install_dir : get_option('datadir') / 'licenses',
    rename : meson.project_name() + '.LICENSE.txt'
)
