src = [
    'src/driver.cpp'
]

inc = include_directories('include')

libryzenmaster = library('ryzenmaster', src,
    include_directories : inc,
)

ryzenmaster = declare_dependency(
    link_with: libryzenmaster,
    include_directories: inc,
)

exe = executable('ryzenmaster-test', 'test/connect.cpp',
    dependencies: [ ryzenmaster ],
)
