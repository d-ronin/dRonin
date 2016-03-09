"""
Crude implementation of struct.Struct, intended for pyjs use

Copyright (C) 2016 dRonin, http://dronin.org

Licensed under the GNU LGPL version 2.1 or any later version (see COPYING.LESSER)
"""

import struct

def calcsize(fmt_string):
    return struct.calcsize(fmt_string)

class Struct:
    def __init__(self, fmt_string):
        self.fmt_string = fmt_string
        self.size = calcsize(fmt_string)

    def pack(self, *args):
        return struct.pack(self.fmt_string, *args)

    def unpack(self, buff):
        return struct.unpack(self.fmt_string, buff)

    def unpack_from(self, buff, offset):
        return struct.unpack_from(self.fmt_string, buff, offset)
