"""A setuptools based setup module.
See:
https://packaging.python.org/en/latest/distributing.html
https://github.com/pypa/sampleproject
"""

# Always prefer setuptools over distutils
from setuptools import setup, find_packages
# To use a consistent encoding
from codecs import open
from os import path

here = path.abspath(path.dirname(__file__))
with open(path.join(here, 'README.rst'), encoding='utf-8') as f:
    long_description = f.read()

long_description = """
This is the dRonin UAVTalk API.  With these modules, it is possible to
communicate with dRonin flight controllers and interpret log files and
settings partitions.
"""

setup(
    name='dronin',

    # Versions should comply with PEP440.  For a discussion on single-sourcing
    # the version across setup.py and the project code, see
    # https://packaging.python.org/en/latest/single_source_version.html
    version='20170305',

    description='dRonin Python API',
    long_description=long_description,

    # The project's main homepage.
    url='http://dronin.org/',

    # Author details
    author='dronin',
    author_email='info@dronin.org',

    # Choose your license
    license='LGPLv2+',

    # See https://pypi.python.org/pypi?%3Aaction=list_classifiers
    classifiers=[
        # How mature is this project? Common values are
        #   3 - Alpha
        #   4 - Beta
        #   5 - Production/Stable
        'Development Status :: 4 - Beta',

        # Indicate who your project is intended for
        'Intended Audience :: Developers',
        'Topic :: Software Development :: Embedded Systems',
        "Topic :: Software Development :: Object Brokering",


        # Pick your license as you wish (should match "license" above)
        'License :: OSI Approved :: GNU Lesser General Public License v2 or later (LGPLv2+)',

        # Specify the Python versions you support here. In particular, ensure
        # that you indicate whether you support Python 2, Python 3 or both.
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.3',
        'Programming Language :: Python :: 3.4',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6'
    ],

    # What does your project relate to?
    keywords='dronin flight controller quadcopter',

    # You can just specify the packages manually here if your project is
    # simple. Or you can use find_packages().
    packages = ['dronin', 'dronin.logviewer'],

    # Just requires the base python system to run
    install_requires=[],

    # 'all' target requires python3 because there are only PyQt5 wheels for
    # python3.
    extras_require={
        'all': ['pyserial', 'numpy', 'matplotlib', 'dronin-pyqtgraph', 'PyQt5'],
    },

    scripts = [ 'dronin-dumplog', 'dronin-halt',
        'dronin-getconfig', 'dronin-logfsimport',
        'dronin-shell' ],
#    package_data={
#        'sample': ['package_data.dat'],
#    },

#    data_files=[('my_data', ['data/data_file'])],

    # To provide executable scripts, use entry points in preference to the
    # "scripts" keyword. Entry points provide cross-platform support and allow
    # pip to create the appropriate form of executable for the target platform.
    entry_points={
        'gui_scripts': [
            'dronin-logview=dronin.logviewer.logviewer:main',
        ],
    },
)
