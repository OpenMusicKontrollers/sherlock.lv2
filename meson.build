project('sherlock.lv2', 'c', default_options : [
	'buildtype=release',
	'warning_level=1',
	'werror=false',
	'b_lto=false',
	'c_std=c11'])

build_root = meson.build_root()
static_link = meson.is_cross_build()

cc = meson.get_compiler('c')

m_dep = cc.find_library('m')
lv2_dep = dependency('lv2', version : '>=1.14.0')
sratom_dep = dependency('sratom-0', version : '>=0.6.0', static : static_link)

dsp_deps = [m_dep, lv2_dep]
ui_deps = [m_dep, lv2_dep, sratom_dep]
ui_links = []

pugl_inc = include_directories('pugl')
props_inc = include_directories('props.lv2')
osc_inc = include_directories('osc.lv2')
ser_inc = include_directories('ser_atom.lv2')
inc_dir = [pugl_inc, props_inc, osc_inc, ser_inc]

inst_dir = join_paths(get_option('libdir'), 'lv2', meson.project_name())

rawvers = run_command('cat', 'VERSION').stdout().strip()
version = rawvers.split('.')

conf_data = configuration_data()
conf_data.set('MAJOR_VERSION', version[0])
conf_data.set('MINOR_VERSION', version[1])
conf_data.set('MICRO_VERSION', version[2])

add_project_arguments('-DSHERLOCK_VERSION="'+rawvers+'"', language : 'c')
add_project_arguments('-D_GNU_SOURCE', language : 'c')
add_project_arguments('-DPUGL_HAVE_GL', language : 'c')

flex = find_program('flex')
lgen = generator(flex,
	output : '@PLAINNAME@.c',
	arguments : ['--prefix=enc', '-o', '@OUTPUT@', '@INPUT@'])

cp = find_program('cp')
lv2_validate = find_program('lv2_validate', native : true, required : false)
sord_validate = find_program('sord_validate', native : true, required : false)
lv2lint = find_program('lv2lint', required : false)
clone = [cp, '@INPUT@', '@OUTPUT@']

lfiles = lgen.process('encoder.l')

dsp_srcs = ['sherlock.c',
	'atom_inspector.c',
	'midi_inspector.c',
	'osc_inspector.c']

ui_srcs = ['sherlock_nk.c',
	'atom_inspector_nk.c',
	'midi_inspector_nk.c',
	'osc_inspector_nk.c',
	lfiles]

c_args = ['-fvisibility=hidden',
	'-ffast-math']

if host_machine.system() == 'windows'
	add_languages('cpp')
	conf_data.set('UI_TYPE', 'WindowsUI')
	ui_deps += cc.find_library('opengl32')
	ui_deps += cc.find_library('gdi32')
	ui_deps += cc.find_library('user32')
	ui_srcs += 'pugl/pugl/pugl_win.cpp'
elif host_machine.system() == 'darwin'
	add_languages('objc')
	conf_data.set('UI_TYPE', 'CocoaUI')
	ui_links += ['-framework', 'OpenGL']
	ui_links += ['-framework', 'Cocoa']
	ui_srcs += 'pugl/pugl/pugl_osx.m'
else
	conf_data.set('UI_TYPE', 'X11UI')
	ui_deps += dependency('gl')
	ui_deps += dependency('x11', version : '>=1.6.0')
	ui_deps += dependency('xext', version : '>=1.3.0')
	ui_srcs += 'pugl/pugl/pugl_x11.c'
endif

mod = shared_module('sherlock', dsp_srcs,
	c_args : c_args,
	include_directories : inc_dir,
	name_prefix : '',
	dependencies : dsp_deps,
	install : true,
	install_dir : inst_dir)

ui = shared_module('sherlock_nk', ui_srcs,
	c_args : c_args,
	include_directories : inc_dir,
	name_prefix : '',
	dependencies : ui_deps,
	link_args : ui_links,
	install : true,
	install_dir : inst_dir)

suffix = mod.full_path().strip().split('.')[-1]
conf_data.set('MODULE_SUFFIX', '.' + suffix)

manifest_ttl = configure_file(input : 'manifest.ttl.in', output : 'manifest.ttl',
	configuration : conf_data,
	install : true,
	install_dir : inst_dir)

dsp_ttl = custom_target('dsp_ttl',
	input : 'sherlock.ttl',
	output : 'sherlock.ttl',
	command : clone,
	install : true,
	install_dir : inst_dir)

ui_ttl = custom_target('ui_ttl',
	input : 'sherlock_ui.ttl',
	output : 'sherlock_ui.ttl',
	command : clone,
	install : true,
	install_dir : inst_dir)

custom_target('font',
	input : join_paths('nuklear', 'extra_font', 'Cousine-Regular.ttf'),
	output : 'Cousine-Regular.ttf',
	command : clone,
	install : true,
	install_dir : inst_dir)

if lv2_validate.found() and sord_validate.found()
	test('LV2 validate', lv2_validate,
		args : [manifest_ttl, dsp_ttl, ui_ttl])
endif

if lv2lint.found()
	test('LV2 lint', lv2lint,
		args : ['-Ewarn', '-I', join_paths(build_root, ''),
			'http://open-music-kontrollers.ch/lv2/sherlock#atom_inspector',
			'http://open-music-kontrollers.ch/lv2/sherlock#midi_inspector',
			'http://open-music-kontrollers.ch/lv2/sherlock#osc_inspector'])
endif
