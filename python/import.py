#!/usr/bin/env python

# Proof of concept example of using the logfs api.
from dronin.logfs import LogFSImport

def main():
    import argparse

    parser = argparse.ArgumentParser(description="Import a logfs settings partition and convert to XML")
    parser.add_argument('filename', metavar='filename.bin', help="the filename in which the settings dump is stored")
    parser.add_argument('-g', dest='githash', metavar='githash', help="the githash to use to interpret the settings dump", default="next")

    arg = parser.parse_args()

    githash = arg.githash
    srcfile = arg.filename

    imported = LogFSImport(githash, file(srcfile, 'rb').read())

    print imported.ExportXML()

if __name__ == '__main__':
        main()
