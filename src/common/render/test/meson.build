# awful
fs.copyfile(dstorage.get_variable('bindir') / 'dstorage.dll')
subdir('redist')
subdir('redist/d3d12')

testcases = {
    'Setup render instance': 'instance.cpp',
    'Save render device data': 'store.cpp',

    'Create render next CoreContext': 'next/context.cpp',
    'Create render next CoreContext with debug features': 'next/context-debug.cpp',
}

foreach name, source : testcases
    exe = executable('test-render-' + name.to_lower().replace(' ', '-'), source,
        dependencies : [ render, coretest, dbtest ]
    )

    test(name, exe,
        suite : 'render',
        kwargs : testkwargs
    )
endforeach
