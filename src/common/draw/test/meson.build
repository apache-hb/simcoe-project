# awful
fs.copyfile(dstorage.get_variable('bindir') / 'dstorage.dll')
subdir('redist')
subdir('redist/d3d12')

testcases = {
    'Setup Draw Context': 'context.cpp',
}

foreach name, source : testcases
    exe = executable('test-draw-' + name.to_lower().replace(' ', '-'), source,
        dependencies : [ render, draw, coretest ]
    )

    test(name, exe,
        suite : 'draw',
        kwargs : testkwargs + { 'timeout' : 30 }
    )
endforeach
