project('compressonator')

dir = meson.current_source_dir()

cli = find_program('compressonatorcli.exe', dirs : dir, native : true)

meson.override_find_program('compressonatorcli', cli)
