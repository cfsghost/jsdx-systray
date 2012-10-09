{
	'targets': [
		{
			'target_name': 'jsdx_systray',
			'sources': [
				'src/jsdx_systray.cpp',
				'src/ewmh.cpp',
				'src/xembed.cpp',
				'src/x11.cpp'
			],
			'conditions': [
				['OS=="linux"', {
					'cflags': [
						'<!@(pkg-config --cflags x11 xrender)'
					],
					'ldflags': [
						'<!@(pkg-config  --libs-only-L --libs-only-other x11 xrender)'
					],
					'libraries': [
						'<!@(pkg-config  --libs-only-l --libs-only-other x11 xrender)'
					],
					'defines': [
						'USE_X11'
					]
				}]
			]
		}
	]
}
