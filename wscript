import Options
from os import unlink, symlink, popen
from os.path import exists

srcdir = "."
blddir = "build"
VERSION = "0.0.1"

def set_options(opt):
	opt.tool_options("compiler_cxx")
	opt.add_option('--with-x11', action='store', default=1, help='Enable X11 backend support [Default: True]')

def configure(conf):
	conf.check_tool("compiler_cxx")
	conf.check_tool("node_addon")

	if Options.options.with_x11:
		print "Enable X11 backend support"
		conf.env["WITH_X11"] = True
		conf.check_cfg(package='x11', uselib_store='X11', args='--cflags --libs')

def build(bld):
	obj = bld.new_task_gen("cxx", "shlib", "node_addon")
	obj.cxxflags = ["-Wall", "-ansi", "-pedantic"]
	obj.target = "jsdx_systray"
	obj.source = """
		src/jsdx_systray.cpp
		"""
	obj.cxxflags = ["-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE"]
	obj.uselib = "XRANDR"

	if bld.env["WITH_X11"]:
		obj.cxxflags.append("-DUSE_X11");

		obj.uselib += " X11"

def shutdown():
	if Options.commands['clean']:
		if exists('jsdx_systray.node'): unlink('jsdx_systray.node')
	else:
		if exists('build/default/jsdx_systray.node') and not exists('jsdx_systray.node'):
			symlink('build/default/jsdx_systray.node', 'jsdx_systray.node')
