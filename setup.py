"""setuptools config for wallycore """
from setuptools import setup

import os
import subprocess

from distutils.command.build_clib import build_clib as _build_clib

class build_clib(_build_clib):
    def run(self):
        abs_path = os.path.dirname(os.path.abspath(__file__))
        cln_cmd = [abs_path+'/tools/cleanup.sh']
        ppr_cmd = [abs_path+'/tools/autogen.sh']
        cmd = [abs_path+"/configure", "--enable-swig-python"]
        subprocess.check_call(cln_cmd, cwd=abs_path)
        subprocess.check_call(ppr_cmd, cwd=abs_path)
        subprocess.check_call(cmd, cwd=abs_path)
        subprocess.check_call(["make"], cwd=abs_path)

setup(
    name='wallycore',

    version='0.0.2',
    description='libwally Bitcoin library',
    long_description='Python bindings for the libwally Bitcoin library',
    url='https://github.com/jgriffiths/libwally-core',
    author='Jon Griffiths',
    author_email='jon_p_griffiths@yahoo.com',
    license='MIT',
    zip_safe=False,
    libraries=[('wallycore',{'sources':['include/wally_core.h']})],
    cmdclass={
        'build_clib': build_clib,
    },

    classifiers=[
        'Development Status :: 3 - Alpha',

        'Intended Audience :: Developers',
        'Topic :: Software Development :: Libraries',

        'License :: OSI Approved :: MIT License',

        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3.5',
    ],

    keywords='Bitcoin wallet BIP32 BIP38 BIP39 secp256k1',

    packages=['wallycore'],
    package_dir={'':'src/swig_python'},
    data_files=[('', ['src/.libs/libwallycore.so'])] ,
)
