from setuptools import setup, Extension
import pybind11
import numpy
 
ext = Extension(
    "ring_buffer",
    sources=["ring_buffer.cpp"],
    include_dirs=[pybind11.get_include(), numpy.get_include()],
    language="c++",
    extra_compile_args=["-std=c++17", "-O0"],
)
setup(name="ring_buffer", ext_modules=[ext])
