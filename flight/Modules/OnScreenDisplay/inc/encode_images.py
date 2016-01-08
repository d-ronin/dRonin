#!/usr/bin/env python

from string import Template
from itertools import izip, chain, repeat

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.image as mpimg

img_c_typedef = """

struct Image {
\tuint16_t width;
\tuint16_t height;
\tuint8_t* mask;
\tuint8_t* level;
};

"""

img_c_template = Template("""
static const struct Image image_$name = {
\t.width = $width,
\t.height = $height,
\t.mask = (uint8_t*)mask_$name,
\t.level = (uint8_t*)level_$name,
};

""")

def grouper(n, iterable, padvalue=None):
    "grouper(3, 'abcdefg', 'x') --> ('a','b','c'), ('d','e','f'), ('g','x','x')"
    return izip(*[chain(iterable, repeat(padvalue, n-1))]*n)

def encode_image(fname):
    img = img=mpimg.imread(fname)
    height, width, depth = img.shape
    print 'Encoding %s: %d x %d pixels' % (fname, width, height)
    gray = np.max(img[:, :, :3], axis=-1)
    bw = gray > 0.9

    if depth == 4:
        alpha = img[:, :, -1]
        mask = alpha > 0.5
    elif depth == 3:
        mask = np.logical_or(gray < 0.1, gray > 0.9)

    # width in bytes
    byte_width = int(np.ceil(width / float(8)))
    mask_buffer = np.zeros((height, byte_width), dtype=np.uint8)
    level_buffer = np.zeros((height, byte_width), dtype=np.uint8)

    for row in xrange(height):
        for col in xrange(width):
            if mask[row, col]:
                pos = col / 8
                mask_buffer[row, pos] |= 0x01 << (7 - (col % 8))
                level_buffer[row, pos] |= bw[row, col] << (7 - (col % 8))
    image = dict(width=8*byte_width, height=height, mask=mask_buffer, level=level_buffer)
    return image





def c_format_image(fid, name, image, row_width=16):
    """Format image as a C header"""
    for buf in ['mask', 'level']:
        fid.write('static const uint8_t %s_%s[] = {\n' % (buf, name))
        n_rows = np.ceil(len(image[buf].ravel()) / float(row_width))
        for ii, row in enumerate(grouper(row_width, image[buf].ravel())):
            row = [val for val in row if val is not None]
            fid.write('\t')

            for byte in row[:-1]:
                fid.write('0x%0.2x, ' % byte)
            if ii < n_rows - 1:
                fid.write('0x%0.2x,\n' % row[-1])
            else:
                fid.write('0x%0.2x\n};\n' % row[-1])


if __name__ == '__main__':
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option('-o', '--out', dest='filename_out',
                      help='Output file', metavar='FILE')

    options, args = parser.parse_args()

    in_files = args
    print 'Input images: %s' % str(in_files)
    with open(options.filename_out, 'w') as fid:
        fid.write('#ifndef IMAGES_H\n#define IMAGES_H\n')
        fid.write(img_c_typedef)
        for fname in in_files:
            image = encode_image(fname)
            c_format_image(fid, fname[:-4], image)

            fid.write(img_c_template.substitute(width=image['width'],
                                                height=image['height'],
                                                name=fname[:-4]))
        fid.write('#endif /* IMAGES_H */\n')
