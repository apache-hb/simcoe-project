src = [
    'src/entity.cpp',
    'src/game.cpp',
    'src/object.cpp',

    meta_tool.process('include/game/object.hpp', extra_args : [ 'game' ]),
    meta_tool.process('include/game/entity.hpp', extra_args : [ 'game' ])
]

inc = include_directories('include')
jolt = dependency('jolt')

deps = [ input, draw, logs, world, meta, jolt ]

libgame = library('game', src,
    include_directories : [ inc, 'include/game' ],
    dependencies : deps
)

game = declare_dependency(
    link_with : [ libgame ],
    include_directories : [ inc, libgame.private_dir_include() ],
    dependencies : deps
)
