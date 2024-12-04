audio_include = include_directories('include')

src = [
    'src/audio.cpp'
]

deps = [
    core,
    cpp.find_library('xaudio2')
]

libaudio = library('audio', src,
    include_directories : audio_include,
    dependencies : deps
)

audio = declare_dependency(
    link_with : libaudio,
    include_directories : audio_include,
    dependencies : [ core ]
)
