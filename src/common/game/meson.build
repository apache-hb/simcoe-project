src = [
    'src/entity.cpp',
    'src/game.cpp',

    meta_tool.process('include/game/entity.hpp', extra_args : [ 'game' ])
]

inc = include_directories('include')

deps = [ input, render, logs, world, meta, flecs ]

libgame = library('game', src,
    include_directories : [ inc, 'include/game' ],
    dependencies : deps
)

game = declare_dependency(
    link_with : [ libgame ],
    include_directories : [ inc ],
    dependencies : deps
)
