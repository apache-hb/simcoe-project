project('directx-tex', 'cpp',
    license : 'MIT',
    license_files : [ 'LICENSE' ]
)

src = [
    'DirectXTex/BC.cpp',
    'DirectXTex/BC4BC5.cpp',
    'DirectXTex/BC6HBC7.cpp',
    'DirectXTex/DirectXTexCompress.cpp',
    'DirectXTex/DirectXTexConvert.cpp',
    'DirectXTex/DirectXTexD3D11.cpp',
    'DirectXTex/DirectXTexD3D12.cpp',
    'DirectXTex/DirectXTexDDS.cpp',
    'DirectXTex/DirectXTexFlipRotate.cpp',
    'DirectXTex/DirectXTexHDR.cpp',
    'DirectXTex/DirectXTexImage.cpp',
    'DirectXTex/DirectXTexMipmaps.cpp',
    'DirectXTex/DirectXTexMisc.cpp',
    'DirectXTex/DirectXTexNormalMaps.cpp',
    'DirectXTex/DirectXTexPMAlpha.cpp',
    'DirectXTex/DirectXTexResize.cpp',
    'DirectXTex/DirectXTexTGA.cpp',
    'DirectXTex/DirectXTexUtil.cpp',
    'DirectXTex/DirectXTexWIC.cpp',
]

directxtex = static_library('directx-tex', src)

dxtex_dep = declare_dependency(
    link_with : directxtex,
    include_directories : '.'
)

meson.override_dependency('directx-tex', dxtex_dep)
