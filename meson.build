project(
    'cerberus',
    'c',
    version: '0.0.1',
	license: 'MIT',
	meson_version: '>=0.48.0',
	default_options: [
		'c_std=c11',
		'warning_level=2',
		'werror=true',
    ]
)

wayland_client = dependency('wayland-client')
wayland_protos = dependency('wayland-protocols', version: '>=1.14')
gtkdep = dependency('gtk+-3.0')

git = find_program('git', required: false)
wayland_scanner = find_program('wayland-scanner')

wl_protocol_dir = wayland_protos.get_pkgconfig_variable('pkgdatadir')

if wayland_client.version().version_compare('>=1.14.91')
    code_type = 'private-code'
else
    code_type = 'code'
endif

wayland_scanner_code = generator(
    wayland_scanner,
    output: '@BASENAME@-protocol.c',
    arguments: [code_type, '@INPUT@', '@OUTPUT@'],
)

wayland_scanner_client = generator(
    wayland_scanner,
    output: '@BASENAME@-client-protocol.h',
    arguments: ['client-header', '@INPUT@', '@OUTPUT@'],
)

client_protos_src = []
client_protos_headers = []

client_protocols = [
    [wl_protocol_dir, 'stable/xdg-shell/xdg-shell.xml'],
    [wl_protocol_dir, 'unstable/xdg-output/xdg-output-unstable-v1.xml'],
]

foreach p : client_protocols
    xml = join_paths(p)
    client_protos_src += wayland_scanner_code.process(xml)
    client_protos_headers += wayland_scanner_client.process(xml)
endforeach

lib_client_protos = static_library(
    'client_protos',
    client_protos_src + client_protos_headers,
    dependencies: [wayland_client]
) # for the include directory

client_protos = declare_dependency(
    link_with: lib_client_protos,
    sources: client_protos_headers,
)

conf_data = configuration_data()

subdir('include')

dependencies = [
    client_protos,
    gtkdep,
    wayland_client,
]

sources = [
    'main.c',
]

swaybg_inc = include_directories('include')

executable('cerberus',
    sources,
    include_directories: [],
    dependencies: dependencies,
    install: true
)