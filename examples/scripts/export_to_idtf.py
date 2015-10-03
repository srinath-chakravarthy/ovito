""" 
    This OVITO script exports the particle to the IDTF file format, 
    which can be converted to the u3d format, which in turn can be 
    embedded in PDF documents. 
"""
import sys

from ovito import *
from ovito.data import *
