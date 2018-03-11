#!/usr/bin/env python3

from string import Template
from itertools import chain, repeat

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.image as mpimg

img_c_typedef_split = """

struct Image {
\tuint16_t width;
\tuint16_t height;
\tuint8_t* mask;
\tuint8_t* level;
};

"""

img_c_template_split = Template("""
static const struct Image image_$name = {
\t.width = $width,
\t.height = $height,
\t.mask = (uint8_t*)mask_$name,
\t.level = (uint8_t*)level_$name,
};

""")

img_c_typedef = """

struct Image {
\tuint16_t width;
\tuint16_t height;
\tuint8_t* data;
};

"""

img_c_template = Template("""
static const struct Image image_$name = {
\t.width = $width,
\t.height = $height,
\t.data = (uint8_t*)data_$name,
};

""")

def grouper(n, iterable, padvalue=None):
    "grouper(3, 'abcdefg', 'x') --> ('a','b','c'), ('d','e','f'), ('g','x','x')"
    return zip(*[chain(iterable, repeat(padvalue, n-1))]*n)

def encode_image(fname):
    img = img=mpimg.imread(fname)
    height, width, depth = img.shape
    print('Encoding %s: %d x %d pixels' % (fname, width, height))
    gray = np.max(img[:, :, :3], axis=-1)
    bw = gray > 0.9

    if depth == 4:
        alpha = img[:, :, -1]
        mask = alpha > 0.5
    elif depth == 3:
        mask = np.logical_or(gray < 0.1, gray > 0.9)

    # width in bytes
    byte_width_split = int(np.ceil(width / float(8)))
    # buffers for OSDs with split level/mask buffers
    mask_buffer = np.zeros((height, byte_width_split), dtype=np.uint8)
    level_buffer = np.zeros((height, byte_width_split), dtype=np.uint8)
    # buffer for OSDs with a single buffer
    byte_width = int(np.ceil(width / float(4)))
    data_buffer = np.zeros((height, byte_width), dtype=np.uint8)

    for row in range(height):
        for col in range(width):
            if mask[row, col]:
                pos = int(col / 8)
                mask_buffer[row, pos] |= 0x01 << (7 - (col % 8))
                level_buffer[row, pos] |= bw[row, col] << (7 - (col % 8))
                pos = int(col / 4)
                data_buffer[row, pos] |= (bw[row, col] << (7 - 2 * (col % 4))) | (0x01 << (6 - 2 * (col % 4)))
    image = dict(width=4*byte_width, width_split=8*byte_width_split, height=height, mask=mask_buffer,
                 level=level_buffer, data=data_buffer)
    return image



def c_format_image(fid, name, image, buffers, row_width=16):
    """Format image as a C header"""
    for buf in buffers:
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
    print('Input images: %s' % str(in_files))
    images = dict()
    for fname in in_files:
        images[fname[:-4]] = encode_image(fname)
  
    with open(options.filename_out, 'w') as fid:
        fid.write('#ifndef IMAGES_H\n#define IMAGES_H\n')
        fid.write('#if defined(PIOS_VIDEO_SPLITBUFFER)\n')
        fid.write(img_c_typedef_split)
        for name, image in images.items():
            c_format_image(fid, name, image, buffers=('level', 'mask'))
            fid.write(img_c_template_split.substitute(width=image['width_split'],
                                                      height=image['height'], name=name))
        fid.write('#else /* defined(PIOS_VIDEO_SPLITBUFFER) */\n')
        fid.write(img_c_typedef)
        for name, image in images.items():
            c_format_image(fid, name, image, buffers=('data',))
            fid.write(img_c_template.substitute(width=image['width'],
                                                height=image['height'], name=name))
        fid.write('#endif /* defined(PIOS_VIDEO_SPLITBUFFER) */\n')
        fid.write('#endif /* IMAGES_H */\n')
