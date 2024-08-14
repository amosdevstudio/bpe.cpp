from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension, build_ext

ext_modules = [
    Pybind11Extension(
        "pybpe",
        ["src/pywrap.cpp", "src/bpe.cpp"],
        include_dirs=["src/"],
        extra_compile_args=['-std=c++11', '-O3'],
        extra_link_args=['-lpcre2-8'],
    ),
]

setup(
    name="pybpe",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
)
