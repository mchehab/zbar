#!/usr/bin/env python
#------------------------------------------------------------------------
#  Copyright 2019 (c) Mauro Carvalho Chehab <mchehab+samsung@kernel.org>
#
#  This file is part of the ZBar Bar Code Reader.
#
#  The ZBar Bar Code Reader is free software; you can redistribute it
#  and/or modify it under the terms of the GNU Lesser Public License as
#  published by the Free Software Foundation; either version 2.1 of
#  the License, or (at your option) any later version.
#
#  The ZBar Bar Code Reader is distributed in the hope that it will be
#  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
#  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser Public License for more details.
#
#------------------------------------------------------------------------

from __future__ import print_function
import zbar, sys

try:
    from PIL import Image
except:
    try:
        import Image
    except:
        print("No image library on python. Aborting test")
        sys.exit()

if len(sys.argv) < 1 or len(sys.argv) > 3:
    print("Usage: %s <file name> [<expected text to check>]")
    sys.exit(-1)

filename = sys.argv[1]

org_image = Image.open(filename)

image = org_image.convert(mode='L')

width = image.size[0]
height = image.size[1]
raw_data = image.tobytes()

scanner = zbar.ImageScanner()
image = zbar.Image(width=width, height=height, format='Y800', data=raw_data)
scanner.scan(image)

if len(sys.argv) == 3:
    text = sys.argv[2]

    found = False
    for symbol in image:
        found = True
        if symbol.data == text:
            print("OK")
        else:
            print("Expecting %s, received %s" % (text, symbol.data))

    if not found:
        print("Can't process file")
else:
    for symbol in image:
        print("Decoded as %s" % symbol.data)
