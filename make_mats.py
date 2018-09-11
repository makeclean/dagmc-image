#!/usr/env/python

from pyne.material import Material
from pyne.material import MaterialLibrary

mat1 = Material({'Fe':1.0},density=1.0)
mat1 = mat1.expand_elements()
mat2 = Material({'Cu':1.0},density=1.0)
mat2 = mat2.expand_elements()
mat3 = Material({'C':1.0},density=1.0)
mat3 = mat3.expand_elements()

mat_lib = MaterialLibrary()
mat_lib["M1"] = mat1
mat_lib["M2"] = mat2
mat_lib["M3"] = mat3
mat_lib.write_hdf5("mat_lib.h5")
