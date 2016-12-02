#!/usr/bin/env python

"""
Interface to read data and do numerical computing on autotune data partitions.

Copyright (C) 2016 dRonin, http://dronin.org
Licensed under the GNU LGPL version 2.1 or any later version (see COPYING.LESSER)
"""

import sys, struct

import pandas as pd

at_flash_header_fmt = struct.Struct('<QHHHH')
at_measurement_fmt = struct.Struct('<ffffff')

def process_autotune_lump(contents):
    """ Given a block of autotune data, return a time-series panda dataseries """
    hdr_tup = at_flash_header_fmt.unpack_from(contents)

    if hdr_tup[0] != 0x656e755480008041:
        print("Invalid magic")
        sys.exit(1)

    offset = at_flash_header_fmt.size

    pts = []

    time_step = 0.001   # Assume 1KHz sampling rate for now

    if (hdr_tup[3]):
        time_step = 1.0 / hdr_tup[3]

    print("Autotune data: pts = %d, aux_data_len = %d, timestep = %0.2fms"% (
            hdr_tup[1], hdr_tup[2], time_step * 1000))

    for i in range(hdr_tup[1]):
        d_tup = at_measurement_fmt.unpack_from(contents, offset)

        d_tup = (i * time_step,) + d_tup

        pts.append(d_tup)

        offset += at_measurement_fmt.size

    return pd.DataFrame.from_records(data = pts, index = 'time',
            columns = [ 'time', 'gyroX', 'gyroY', 'gyroZ',
                'desiredX', 'desiredY', 'desiredZ' ])


def read_autotune_lump(f_name):
    """ Given a filename with autotune data, return a time-series panda dataseries """
    with open(f_name, 'rb') as f:
        contents = f.read()

    return process_autotune_lump(contents)

def main(argv):
    if len(argv) != 1:
        print("nope")
        sys.exit(1)

    frame = read_autotune_lump(argv[0])

    print(frame)

if __name__ == "__main__":
    main(sys.argv[1:])
