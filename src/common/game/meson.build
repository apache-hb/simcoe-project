src = [
    'src/entity.cpp',
    'src/game.cpp',
    'src/object.cpp',
    'src/camera.cpp'
]

inc = include_directories('include')

deps = [
    input, draw, logs, world,

    dependency('jolt')
]

libgame = library('game', src,
    include_directories : [ inc ],
    dependencies : deps
)

game = declare_dependency(
    link_with : [ libgame ],
    include_directories : [ inc ],
    dependencies : deps
)
