#!/usr/bin/env python3

"""
Interface to read data and do numerical computing on autotune data partitions.

Copyright (C) 2016 dRonin, http://dronin.org
Licensed under the GNU LGPL version 2.1 or any later version (see COPYING.LESSER)
"""

from __future__ import print_function

import sys, struct

import pandas as pd
from scipy import signal

at_flash_header_fmt = struct.Struct('<QHHHH')
at_measurement_fmt = struct.Struct('<ffffff')

def process_autotune_lump(contents, filter_freq=55, filter_order=0):
    """ Given a block of autotune data, return a time-series panda dataseries """
    hdr_tup = at_flash_header_fmt.unpack_from(contents)

    if hdr_tup[0] != 0x656e755480008041:
        raise ValueError

    offset = at_flash_header_fmt.size

    pts = []

    time_step = 0.001   # Assume 1KHz sampling rate for now

    if (hdr_tup[3]):
        time_step = 1.0 / hdr_tup[3]

    print("Autotune data: pts = %d, aux_data_len = %d, timestep = %0.2fms"% (
            hdr_tup[1], hdr_tup[2], time_step * 1000))

    for i in range(hdr_tup[1]):
        d_tup = at_measurement_fmt.unpack_from(contents, offset)

        d_tup = d_tup + (i * time_step, )

        pts.append(d_tup)

        offset += at_measurement_fmt.size

    # Don't set the index = 'time' for now, as it often makes things more of a
    # pain for the caller than it helps
    frame = pd.DataFrame.from_records(data = pts,
            columns = [ 'gyroX', 'gyroY', 'gyroZ',
                'desiredX', 'desiredY', 'desiredZ', 'time' ])

    # Double the data before filtering and differentiating
    frame = pd.concat([frame, frame], ignore_index=True)

    # corresponds to 55Hz gyrocutoff
    if filter_order > 0:
        b, a = signal.iirfilter(filter_order, [2 * filter_freq * time_step], btype='lowpass', ftype='butter')

        for col in ['X', 'Y', 'Z']:
            filtered = signal.lfilter(b, a, frame['gyro' + col], axis=0)
            frame['filtered' + col] = pd.Series(filtered)
    else:
        for col in ['X', 'Y', 'Z']:
            filtered = frame['gyro' + col]
            frame['filtered' + col] = pd.Series(filtered)

    deriv = frame.diff()

    for col in deriv:
        frame['deriv' + col] = deriv[col]

    # And then take the last half of the data to un-duplicate it
    frame = frame.iloc[int(len(frame) /2):]

    return (frame, time_step)

def read_autotune_lump(f_name, **kwargs):
    """ Given a filename with autotune data, return a time-series panda dataseries """
    try:
        with open(f_name, 'rb') as f:
            contents = f.read()

        return process_autotune_lump(contents, **kwargs)
    except Exception:
        import gzip

        with gzip.open(f_name, 'rb') as f:
            contents = f.read()

        return process_autotune_lump(contents, **kwargs)

def process_autotune_obj(obj, **kwargs):
    raw = obj['Orig']['rawTuneData']

    import base64

    decoded = base64.b64decode(raw)

    import zlib

    tunedata = zlib.decompress(decoded[4:])

    return process_autotune_lump(tunedata, **kwargs)

def process_autotune_json(contents, **kwargs):
    import json

    obj = json.loads(contents)

    return process_autotune_obj(obj, **kwargs)

def read_autotune_json_lump(f_name, **kwargs):
    with open(f_name, 'rb') as f:
        contents = f.read()

    return process_autotune_json(contents, **kwargs)

def read_autotune_from_autotown(tuneid, **kwargs):
    import urllib

    contents = urllib.request.urlopen("http://dronin-autotown.appspot.com/api/tune?tune=%s"%(tuneid)).read()

    return process_autotune_json(contents, **kwargs)

def main(argv):
    if len(argv) != 1:
        print("nope")
        sys.exit(1)

    #frame, time_step = read_autotune_from_autotown('ahFzfmRyb25pbi1hdXRvdG93bnIYCxILVHVuZVJlc3VsdHMYgICAsPibygkM')
    #print(frame)

    frame, time_step = read_autotune_lump(argv[0])

    print(frame)

if __name__ == "__main__":
    main(sys.argv[1:])
