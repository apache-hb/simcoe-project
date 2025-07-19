testcases = {
    'Docker client': 'test/docker.cpp',
}

foreach name, source : testcases
    exe = executable('test-docker-' + name.to_lower().replace(' ', '-'), source,
        dependencies : [ docker, gtest, log_sinks ]
    )

    test(name, exe,
        suite : 'docker',
        kwargs : gtestkwargs
    )
endforeach
