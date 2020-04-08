project('sherlock.lv2', 'c', default_options : [
	'buildtype=release',
	'warning_level=3',
	'werror=false',
	'b_lto=false',
	'c_std=gnu11'])

nk_pugl = subproject('nk_pugl')

lv2libdir = get_option('lv2libdir')

inst_dir = join_paths(lv2libdir, meson.project_name())

nk_pugl_dep = nk_pugl.get_variable('nk_pugl_gl')
cousine_regular_ttf = nk_pugl.get_variable('cousine_regular_ttf')

source_root = meson.source_root()
build_root = meson.build_root()

static_link = meson.is_cross_build()

cc = meson.get_compiler('c')

m_dep = cc.find_library('m')
lv2_dep = dependency('lv2', version : '>=1.14.0')
sratom_dep = dependency('sratom-0', version : '>=0.6.0', static : static_link)

dsp_deps = [m_dep, lv2_dep]
ui_deps = [m_dep, lv2_dep, sratom_dep, nk_pugl_dep]

props_inc = include_directories('props.lv2')
osc_inc = include_directories('osc.lv2')
ser_inc = include_directories('ser_atom.lv2')
nk_pugl_inc = include_directories(join_paths('subprojects', 'nk_pugl'))
inc_dir = [props_inc, osc_inc, ser_inc, nk_pugl_inc]

rawvers = run_command('cat', 'VERSION').stdout().strip()
version = rawvers.split('.')

conf_data = configuration_data()
conf_data.set('MAJOR_VERSION', version[0])
conf_data.set('MINOR_VERSION', version[1])
conf_data.set('MICRO_VERSION', version[2])

add_project_arguments('-DSHERLOCK_VERSION="'+rawvers+'"', language : 'c')
add_project_arguments('-D_GNU_SOURCE', language : 'c')

flex = find_program('flex')
lgen = generator(flex,
	output : '@PLAINNAME@.c',
	arguments : ['--prefix=enc', '-o', '@OUTPUT@', '@INPUT@'])

lv2_validate = find_program('lv2_validate', native : true, required : false)
sord_validate = find_program('sord_validate', native : true, required : false)
lv2lint = find_program('lv2lint', required : false)

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

c_args = [
	'-fvisibility=hidden']

if host_machine.system() == 'windows'
	conf_data.set('UI_TYPE', 'WindowsUI')
elif host_machine.system() == 'darwin'
	conf_data.set('UI_TYPE', 'CocoaUI')
else
	conf_data.set('UI_TYPE', 'X11UI')
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
	install : true,
	install_dir : inst_dir)

suffix = mod.full_path().strip().split('.')[-1]
conf_data.set('MODULE_SUFFIX', '.' + suffix)

manifest_ttl = configure_file(
	input : 'manifest.ttl.in',
	output : 'manifest.ttl',
	configuration : conf_data,
	install : true,
	install_dir : inst_dir)

dsp_ttl = configure_file(
	input : 'sherlock.ttl',
	output : 'sherlock.ttl',
	copy : true,
	install : true,
	install_dir : inst_dir)

ui_ttl = configure_file(
	input : 'sherlock_ui.ttl',
	output : 'sherlock_ui.ttl',
	copy : true,
	install : true,
	install_dir : inst_dir)

configure_file(
	input : cousine_regular_ttf,
	output : 'Cousine-Regular.ttf',
	copy : true,
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
