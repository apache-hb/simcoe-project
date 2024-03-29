src = [
    'src/entity.cpp',
    'src/game.cpp'
]

inc = include_directories('include')

deps = [ input, render, logs, world ]

libgame = library('game', src,
    include_directories : inc,
    dependencies : deps
)
