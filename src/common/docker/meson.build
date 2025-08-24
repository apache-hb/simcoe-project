docker_include = include_directories('include')

src = [
    'src/docker.cpp',
    'src/databases.cpp',
]

curl = dependency('libcurl')
argo = dependency('argo')

deps = [ core, argo, curl, logs ]

libdocker = library('docker', src,
    include_directories: [ docker_include ],
    dependencies : deps
)

docker = declare_dependency(
    link_with : libdocker,
    include_directories : [ docker_include ],
    dependencies : deps
)
