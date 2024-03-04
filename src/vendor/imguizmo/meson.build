src = [
    'GraphEditor.cpp',
    'ImCurveEdit.cpp',
    'ImGradient.cpp',
    'ImGuizmo.cpp',
    'ImSequencer.cpp',
]

inc = include_directories('.')

libimguizmo = library('imguizmo', src,
    include_directories : inc,
    dependencies : imgui,
    override_options : [ 'warning_level=0' ]
)

imguizmo = declare_dependency(
    include_directories : inc,
    link_with : libimguizmo
)
