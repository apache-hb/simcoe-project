# awful
fs.copyfile(dstorage.get_variable('bindir') / 'dstorage.dll')
subdir('redist')
subdir('redist/d3d12')

###
### common render tests
###

stb_image_write = dependency('stb_image_write')

src = [
    'render_test_common.cpp',
]

deps = [ render_base, dbtest, stb_image_write ]

librendertest = library('render-test', src,
    include_directories : [ render_include, reflect_include ],
    dependencies : deps
)

rendertest = declare_dependency(
    link_with : librendertest,
    include_directories : [ render_include ],
    dependencies : deps
)

###
### testcases for current render implementation
###

testcases = {
    'Setup render instance': 'instance.cpp',
    'Save render device data': 'store.cpp',
}

foreach name, source : testcases
    exe = executable('test-render-' + name.to_lower().replace(' ', '-'), source,
        dependencies : [ rendertest, render_core ]
    )

    test(name, exe,
        suite : 'render',
        kwargs : testkwargs + { 'timeout': 10 }
    )
endforeach

###
### testcases for next render implementation
###

testcases = {
    'Create render next CoreContext': 'next/context/create.cpp',
    'Create render next CoreContext with debug features': 'next/context/create_debug.cpp',

    'CoreContext removeDevice is idempotent': 'next/context/remove_device_idempotent.cpp',
    'CoreContext setAdapter is idempotent': 'next/context/set_adapter_idempotent.cpp',
    'CoreContext updateSwapChain is idempotent': 'next/context/update_swapchain_idempotent.cpp',

    'Window swapchain': 'next/swapchain/window.cpp',

    'Swapchain length change': 'next/swapchain/length.cpp',
    'Swapchain size change': 'next/swapchain/resize.cpp',
    'Virtual swapchain': 'next/swapchain/virtual.cpp',
}

foreach name, source : testcases
    exe = executable('test-render-' + name.to_lower().replace(' ', '-'), source,
        dependencies : [ rendertest, render_next ]
    )

    test(name, exe,
        suite : 'render-next',
        kwargs : testkwargs + { 'timeout': 10 }
    )
endforeach
