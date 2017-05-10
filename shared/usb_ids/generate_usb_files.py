#!/usr/bin/env python

from __future__ import print_function

import argparse
from io import TextIOWrapper
import json
import os
import re
import string
import sys

def generate_udev(fp, defs):
    print('# {}'.format(defs['content']), file=fp)
    print('# NOTE: Automatically generated file, DO NOT MODIFY (see shared/usb_ids)', file=fp)

    for group in defs['groups']:
        print('\n#\n# {}\n#\n'.format(group['name']), file=fp)
        for dev in group['devices']:
            if 'comment' in dev:
                print('# {} - {} - {}'.format(group['name'], dev['name'], dev['comment']), file=fp)
            else:
                print('# {} - {}'.format(group['name'], dev['name']), file=fp)

            usb = ('SUBSYSTEM=="usb", ATTR{{idVendor}}=="{:04x}", ATTR{{idProduct}}=="{:04x}", '
                   'MODE="0664", GROUP="plugdev", TAG+="uaccess", TAG+="udev-acl", '
                   'ENV{{ID_MM_DEVICE_IGNORE}}="1"')
            print(usb.format(dev['vid'], dev['pid']), file=fp)

            if 'hid' in dev['type']:
                hidraw = ('KERNEL=="hidraw*", ATTR{{idVendor}}=="{:04x}", ATTR{{idProduct}}=="{:04x}", '
                          'MODE="0664", GROUP="plugdev", TAG+="uaccess", TAG+="udev-acl"')
                print(hidraw.format(dev['vid'], dev['pid']), file=fp)

def generate_inf(fp, defs):
    def _sanitize_name(name):
        return re.sub('[^{}{}]'.format(string.ascii_letters, string.digits), '', name)

    with open(os.path.join(os.path.dirname(__file__), 'driver_template.inf')) as templ_fp:
        devlist = ''
        strings = 'ProviderName = "{}"\n'.format(_sanitize_name(defs['project']))

        for group in defs['groups']:
            devlist += '; {}\n'.format(group['name'])
            for dev in group['devices']:
                if 'cdc' in dev['type']:
                    devlist += '%{}% = DriverInstall,USB\VID_{:04x}&PID_{:04x}&MI_00\n'.format(
                        _sanitize_name(dev['name']), dev['vid'], dev['pid']
                    )
                    strings += '{} = "CDC Driver"\n'.format(_sanitize_name(dev['name']))

        subs = {
            'DEVICE_LIST': devlist,
            'STRINGS': strings,
        }
        templ = string.Template(re.sub('\$[^{]', '$\g<0>', templ_fp.read()))
        print(templ.substitute(subs), file=fp, end='')

def generate_header(fp, defs):
    def _sanitize_name(name):
        return re.sub('[^{}{}]'.format(string.ascii_letters, string.digits), '', name).upper()

    with open(os.path.join(os.path.dirname(__file__), 'header_template.h')) as templ_fp:
        vids = ''
        pids = ''

        for group in defs['groups']:
            for dev in group['devices']:
                if 'comment' in dev:
                    comment = '/* {} - {} - {} */'.format(group['name'], dev['name'], dev['comment'])
                else:
                    comment = '/* {} - {} */'.format(group['name'], dev['name'])
                vids += '\t{}\n'.format(comment)
                vids += '\tDRONIN_VID_{}_{} = 0x{:04x},\n'.format(
                    _sanitize_name(group['name']), _sanitize_name(dev['name']), dev['vid']
                )
                pids += '\t{}\n'.format(comment)
                pids += '\tDRONIN_PID_{}_{} = 0x{:04x},\n'.format(
                    _sanitize_name(group['name']), _sanitize_name(dev['name']), dev['pid']
                )

        subs = {
            'FILENAME': os.path.basename(fp.name),
            'VIDS': vids,
            'PIDS': pids,
        }
        templ = string.Template(templ_fp.read())
        print(templ.substitute(subs), file=fp, end='')

def main():
    parser = argparse.ArgumentParser(description='dRonin USB ID Code Generator')
    parser.add_argument('-i', '--input', type=str, default='./usb_ids.json', help='Default is ./usb_ids.json')
    parser.add_argument('-c', '--c', type=argparse.FileType('w'), default=None, help='File to write C header')
    parser.add_argument('-u', '--udev', type=argparse.FileType('w'), default=None, help='File to write udev rules')
    parser.add_argument('-d', '--driver', type=argparse.FileType('w'), default=None, help='File to write Windows driver inf')
    args = parser.parse_args()

    try:
        with open(args.input, 'r') as fp:
            defs = json.load(fp)
    except IOError as e:
        print('Error: Could not open {}!'.format(args.input), file=sys.stderr)
        parser.print_usage()
        return 1

    if not args.udev and not args.driver and not args.c:
        print('Need at least one output!', file=sys.stderr)
        parser.print_usage()
        return 1

    # turn USB IDs into ints, sadly JSON doesn't allow hex and base-10 ints are ugly to work with for USB IDs
    for group in defs['groups']:
        for dev in group['devices']:
            dev['vid'] = int(dev['vid'], 16)
            dev['pid'] = int(dev['pid'], 16)

    if args.udev:
        generate_udev(args.udev, defs)
        print('Wrote udev rules to {}'.format(args.udev.name))
        args.udev.close()
    if args.driver:
        generate_inf(args.driver, defs)
        print('Wrote Windows driver inf to {}'.format(args.driver.name))
        args.driver.close()
    if args.c:
        generate_header(args.c, defs)
        #print('Wrote C header to {}'.format(args.c.name))
        args.c.close()

    return 0

if __name__ == '__main__':
    sys.exit(main())
