# -*- mode: python -*-

block_cipher = None


a = Analysis(['dronin-logview'],
             pathex=['C:\\Users\\mlyle\\dronin\\python'],
             binaries=None,
             datas=[ ('C:\\Anaconda2\\Library\\bin\\mkl*.dll', '.' ) ],
             hiddenimports=[],
             hookspath=[],
             runtime_hooks=[],
             excludes=['tk', 'tkinter', 'zmq', 'matplotlib', 'PIL', 'IPython', 'tornado', 'jupyter_client', 'pygments', 'jinja2', 'scipy'],
             win_no_prefer_redirects=False,
             win_private_assemblies=False,
             cipher=block_cipher)
pyz = PYZ(a.pure, a.zipped_data,
             cipher=block_cipher)
exe = EXE(pyz,
          a.scripts,
	  exclude_binaries=True,
          name='logview',
          debug=False,
          strip=False,
          upx=True,
          console=True )

coll = COLLECT(exe,
		a.binaries,
		a.zipfiles,
		a.datas,
		strip=False,
		upx=True,
		name='logview')

