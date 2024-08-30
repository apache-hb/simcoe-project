subdir('project')
subdir('editor')

src = [ files('src/main.cpp'), daocc.process('src/editor.xml') ]
pch = meson.current_source_dir() / 'src/stdafx.hpp'

deps = [ editorgui, system, orm, dao ]
