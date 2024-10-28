benchmarks = {
    'Soak test': 'soak.cpp'
}

foreach name, source : benchmarks
    exe = executable('benchmark-draw-' + name.to_lower().replace(' ', '-'), source,
        dependencies : [ draw_next ]
    )

    benchmark(name, exe,
        suite : 'draw',
        kwargs : benchkwargs + { 'timeout' : 30 }
    )
endforeach
