from setuptools import setup, Extension

module = Extension(
    'mdb',
    sources=['mdb.c'],
)

setup(
    name='mdb',
    version='0.1',
    description='C extension for MDB protocol',
    ext_modules=[module],
)
