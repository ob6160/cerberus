project(
  'cerberus',
  'c',
  version: '0.0.1',
	license: 'MIT',
	meson_version: '>=0.48.0',
	default_options: [
		'c_std=c11',
    #   'warning_level=2',
    #		'werror=true',  error on unused vars etc
  ]
)

add_project_arguments([
	'-DWLR_USE_UNSTABLE',
	'-Wall',
], language: 'c')

cc = meson.get_compiler('c')
# Clang complains about some zeroed initializer lists (= {0}), even though they
# are valid
if cc.get_id() == 'clang'
	add_project_arguments('-Wno-missing-field-initializers', language: 'c')
	add_project_arguments('-Wno-missing-braces', language: 'c')
endif

# Avoid wl_buffer deprecation warnings
add_project_arguments('-DWL_HIDE_DEPRECATED', language: 'c')

mcw_inc = include_directories('include')

m_dep           = cc.find_library('m', required : false) 
wlroots         = dependency('wlroots')
wayland_client  = dependency('wayland-client')
wayland_server  = dependency('wayland-server')
glew            = dependency('glew')
glfw            = dependency('glfw3')

executable(
	'cerberus',
	[
		'main.c',
		'log.c',
	],
	include_directories: mcw_inc,
	dependencies: [
		wlroots,
		wayland_server,
    wayland_client,
    glfw,
    glew,
    m_dep,
	],
)
