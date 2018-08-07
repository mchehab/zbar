#!/usr/bin/env python
import sys, os, re
import unittest as ut
import zbar

# FIXME this needs to be conditional
# would be even better to auto-select PIL or ImageMagick (or...)
from PIL import Image

data = None
size = (0, 0)

def load_image():
    image = Image.open(os.path.join(sys.path[0], "barcode.png")).convert('L')
    return(image.tobytes(), image.size)

# FIXME this could be integrated w/fixture creation
(data, size) = load_image()

encoded_widths = \
    '9 111 212241113121211311141132 11111 311213121312121332111132 111 9'

databar_widths = \
    '11 31111333 13911 31131231 11214222 11553 21231313 1'

VIDEO_DEVICE = None
if 'VIDEO_DEVICE' in os.environ:
    VIDEO_DEVICE = os.environ['VIDEO_DEVICE']

is_identifier = re.compile(r'^[A-Z][A-Z_0-9]*$')

class TestZBarFunctions(ut.TestCase):
    def test_version(self):
        ver = zbar.version()
        self.assertTrue(isinstance(ver, tuple))
        self.assertEqual(len(ver), 2)
        for v in ver:
            self.assertTrue(isinstance(v, int))

    def test_verbosity(self):
        zbar.increase_verbosity()
        zbar.set_verbosity(16)

    def test_exceptions(self):
        self.assertTrue(isinstance(zbar.Exception, type))
        for err in (zbar.InternalError,
                    zbar.UnsupportedError,
                    zbar.InvalidRequestError,
                    zbar.SystemError,
                    zbar.LockingError,
                    zbar.BusyError,
                    zbar.X11DisplayError,
                    zbar.X11ProtocolError,
                    zbar.WindowClosed,
                    zbar.WinAPIError):
            self.assertTrue(issubclass(err, zbar.Exception))

    def test_configs(self):
        for cfg in (zbar.Config.ENABLE,
                    zbar.Config.ADD_CHECK,
                    zbar.Config.EMIT_CHECK,
                    zbar.Config.ASCII,
                    zbar.Config.MIN_LEN,
                    zbar.Config.MAX_LEN,
                    zbar.Config.UNCERTAINTY,
                    zbar.Config.POSITION,
                    zbar.Config.X_DENSITY,
                    zbar.Config.Y_DENSITY):
            self.assertTrue(isinstance(cfg, zbar.EnumItem))
            self.assertTrue(int(cfg) >= 0)
            self.assertTrue(is_identifier.match(str(cfg)))

    def test_modifiers(self):
        for mod in (zbar.Modifier.GS1,
                    zbar.Modifier.AIM):
            self.assertTrue(isinstance(mod, zbar.EnumItem))
            self.assertTrue(int(mod) >= 0)
            self.assertTrue(is_identifier.match(str(mod)))

    def test_symbologies(self):
        for sym in (zbar.Symbol.NONE,
                    zbar.Symbol.PARTIAL,
                    zbar.Symbol.EAN8,
                    zbar.Symbol.UPCE,
                    zbar.Symbol.ISBN10,
                    zbar.Symbol.UPCA,
                    zbar.Symbol.EAN13,
                    zbar.Symbol.ISBN13,
                    zbar.Symbol.DATABAR,
                    zbar.Symbol.DATABAR_EXP,
                    zbar.Symbol.I25,
                    zbar.Symbol.CODABAR,
                    zbar.Symbol.CODE39,
                    zbar.Symbol.PDF417,
                    zbar.Symbol.QRCODE,
                    zbar.Symbol.CODE93,
                    zbar.Symbol.CODE128):
            self.assertTrue(isinstance(sym, zbar.EnumItem))
            self.assertTrue(int(sym) >= 0)
            self.assertTrue(is_identifier.match(str(sym)))

    def test_orientations(self):
        for orient in (zbar.Orient.UNKNOWN,
                       zbar.Orient.UP,
                       zbar.Orient.RIGHT,
                       zbar.Orient.DOWN,
                       zbar.Orient.LEFT):
            self.assertTrue(isinstance(orient, zbar.EnumItem))
            self.assertTrue(-1 <= int(orient) <= 3)
            self.assertTrue(is_identifier.match(str(orient)))

class TestScanner(ut.TestCase):
    def setUp(self):
        self.scn = zbar.Scanner()

    def tearDown(self):
        del(self.scn)

    def test_type(self):
        self.assertTrue(isinstance(self.scn, zbar.Scanner))
        self.assertTrue(callable(self.scn.reset))
        self.assertTrue(callable(self.scn.new_scan))
        self.assertTrue(callable(self.scn.scan_y))

        def set_color(color):
            self.scn.color = color
        self.assertRaises(AttributeError, set_color, zbar.BAR)

        def set_width(width):
            self.scn.width = width
        self.assertRaises(AttributeError, set_width, 1)

    # FIXME more scanner tests

class TestDecoder(ut.TestCase):
    def setUp(self):
        self.dcode = zbar.Decoder()

    def tearDown(self):
        del(self.dcode)

    def test_type(self):
        self.assertTrue(isinstance(self.dcode, zbar.Decoder))
        self.assertTrue(callable(self.dcode.set_config))
        self.assertTrue(callable(self.dcode.parse_config))
        self.assertTrue(callable(self.dcode.reset))
        self.assertTrue(callable(self.dcode.new_scan))
        self.assertTrue(callable(self.dcode.set_handler))
        self.assertTrue(callable(self.dcode.decode_width))

        def set_type(typ):
            self.dcode.type = typ
        self.assertRaises(AttributeError, set_type, zbar.Symbol.CODE128)

        def set_color(color):
            self.dcode.color = color
        self.assertRaises(AttributeError, set_color, zbar.BAR)

        def set_data(data):
            self.dcode.data = data
        self.assertRaises(AttributeError, set_data, 'yomama')

        self.assertRaises(AttributeError,
                          self.dcode.__setattr__, 'direction', -1)

    def test_width(self):
        sym = self.dcode.decode_width(5)
        self.assertTrue(sym is zbar.Symbol.NONE)
        self.assertTrue(not sym)
        self.assertEqual(str(sym), 'NONE')

        typ = self.dcode.type
        self.assertTrue(sym is typ)

    def test_reset(self):
        self.assertTrue(self.dcode.color is zbar.SPACE)
        sym = self.dcode.decode_width(1)
        self.assertTrue(self.dcode.color is zbar.BAR)
        self.dcode.reset()
        self.assertTrue(self.dcode.color is zbar.SPACE)
        self.assertEqual(self.dcode.direction, 0)

    def test_decode(self):
        inline_sym = [ -1 ]
        def handler(dcode, closure):
            self.assertTrue(dcode is self.dcode)
            if dcode.type > zbar.Symbol.PARTIAL:
                inline_sym[0] = dcode.type
            closure[0] += 1

        explicit_closure = [ 0 ]
        self.dcode.set_handler(handler, explicit_closure)
        self.dcode.set_config(zbar.Symbol.QRCODE, zbar.Config.ENABLE, 0)

        for (i, width) in enumerate(encoded_widths):
            if width == ' ': continue
            sym = self.dcode.decode_width(int(width))
            if i < len(encoded_widths) - 1:
                self.assertTrue(sym is zbar.Symbol.NONE or
                             sym is zbar.Symbol.PARTIAL)
            else:
                self.assertTrue(sym is zbar.Symbol.EAN13)

        self.assertEqual(self.dcode.configs,
                         set((zbar.Config.ENABLE, zbar.Config.EMIT_CHECK)))
        self.assertEqual(self.dcode.modifiers, set())
        self.assertEqual(self.dcode.data, '6268964977804')
        self.assertTrue(self.dcode.color is zbar.BAR)
        self.assertEqual(self.dcode.direction, 1)
        self.assertTrue(sym is zbar.Symbol.EAN13)
        self.assertTrue(inline_sym[0] is zbar.Symbol.EAN13)
        self.assertEqual(explicit_closure, [ 2 ])

    def test_databar(self):
        self.dcode.set_config(zbar.Symbol.QRCODE, zbar.Config.ENABLE, 0)
        for (i, width) in enumerate(databar_widths):
            if width == ' ': continue
            sym = self.dcode.decode_width(int(width))
            if i < len(databar_widths) - 1:
                self.assertTrue(sym is zbar.Symbol.NONE or
                             sym is zbar.Symbol.PARTIAL)

        self.assertTrue(sym is zbar.Symbol.DATABAR)
        self.assertEqual(self.dcode.get_configs(zbar.Symbol.EAN13),
                         set((zbar.Config.ENABLE, zbar.Config.EMIT_CHECK)))
        self.assertEqual(self.dcode.modifiers, set((zbar.Modifier.GS1,)))
        self.assertEqual(self.dcode.data, '0124012345678905')
        self.assertTrue(self.dcode.color is zbar.BAR)
        self.assertEqual(self.dcode.direction, 1)

    # FIXME test exception during callback

class TestImage(ut.TestCase):
    def setUp(self):
        self.image = zbar.Image(123, 456, 'Y800')

    def tearDown(self):
        del(self.image)

    def test_type(self):
        self.assertTrue(isinstance(self.image, zbar.Image))
        self.assertTrue(callable(self.image.convert))

    def test_new(self):
        self.assertEqual(self.image.format, 'Y800')
        self.assertEqual(self.image.size, (123, 456))
        self.assertEqual(self.image.crop, (0, 0, 123, 456))

        image = zbar.Image()
        self.assertTrue(isinstance(image, zbar.Image))
        self.assertEqual(image.format, '\0\0\0\0')
        self.assertEqual(image.size, (0, 0))
        self.assertEqual(image.crop, (0, 0, 0, 0))

    def test_format(self):
        def set_format(fmt):
            self.image.format = fmt
        self.assertRaises(ValueError, set_format, 10)
        self.assertEqual(self.image.format, 'Y800')
        self.image.format = 'gOOb'
        self.assertEqual(self.image.format, 'gOOb')
        self.assertRaises(ValueError, set_format, 'yomama')
        self.assertEqual(self.image.format, 'gOOb')
        self.assertRaises(ValueError, set_format, 'JPG')
        self.assertEqual(self.image.format, 'gOOb')

    def test_size(self):
        def set_size(sz):
            self.image.size = sz
        self.assertRaises(ValueError, set_size, (1,))
        self.assertRaises(ValueError, set_size, 1)
        self.image.size = (12, 6)
        self.assertRaises(ValueError, set_size, (1, 2, 3))
        self.assertEqual(self.image.size, (12, 6))
        self.assertRaises(ValueError, set_size, "foo")
        self.assertEqual(self.image.size, (12, 6))
        self.assertEqual(self.image.width, 12)
        self.assertEqual(self.image.height, 6)
        self.image.width = 81
        self.assertEqual(self.image.size, (81, 6))
        self.image.height = 64
        self.assertEqual(self.image.size, (81, 64))
        self.assertEqual(self.image.width, 81)
        self.assertEqual(self.image.height, 64)

    def test_crop(self):
        def set_crop(crp):
            self.image.crop = crp
        self.assertRaises(ValueError, set_crop, (1,))
        self.assertRaises(ValueError, set_crop, 1)
        self.image.crop = (1, 2, 100, 200)
        self.assertRaises(ValueError, set_crop, (1, 2, 3, 4, 5))
        self.assertEqual(self.image.crop, (1, 2, 100, 200))
        self.assertRaises(ValueError, set_crop, "foo")
        self.assertEqual(self.image.crop, (1, 2, 100, 200))
        self.image.crop = (-100, -100, 400, 700)
        self.assertEqual(self.image.crop, (0, 0, 123, 456))
        self.image.crop = (40, 50, 60, 70)
        self.assertEqual(self.image.crop, (40, 50, 60, 70))
        self.image.size = (82, 65)
        self.assertEqual(self.image.crop, (0, 0, 82, 65))

class TestImageScanner(ut.TestCase):
    def setUp(self):
        self.scn = zbar.ImageScanner()

    def tearDown(self):
        del(self.scn)

    def test_type(self):
        self.assertTrue(isinstance(self.scn, zbar.ImageScanner))
        self.assertTrue(callable(self.scn.set_config))
        self.assertTrue(callable(self.scn.parse_config))
        self.assertTrue(callable(self.scn.enable_cache))
        self.assertTrue(callable(self.scn.scan))

    def test_set_config(self):
        self.scn.set_config()
        self.assertRaises(ValueError, self.scn.set_config, -1)
        self.assertRaises(TypeError, self.scn.set_config, "yomama")
        self.scn.set_config()

    def test_parse_config(self):
        self.scn.parse_config("disable")
        self.assertRaises(ValueError, self.scn.set_config, -1)
        self.scn.set_config()

class TestImageScan(ut.TestCase):
    def setUp(self):
        self.scn = zbar.ImageScanner()
        self.image = zbar.Image(size[0], size[1], 'Y800', data)

    def tearDown(self):
        del(self.image)
        del(self.scn)

    def test_scan(self):
        n = self.scn.scan(self.image)
        self.assertEqual(n, 1)

        syms = self.image.symbols
        self.assertTrue(isinstance(syms, zbar.SymbolSet))
        self.assertEqual(len(syms), 1)

        i = iter(self.image)
        j = iter(syms)
        self.assertTrue(isinstance(i, zbar.SymbolIter))
        self.assertTrue(isinstance(j, zbar.SymbolIter))
        symi = next(i)
        symj = next(j)
        self.assertRaises(StopIteration, i.__next__)
        self.assertRaises(StopIteration, j.__next__)

        # this is the only way to obtain a Symbol,
        # so test Symbol here
        for sym in (symi, symj):
            self.assertTrue(isinstance(sym, zbar.Symbol))
            self.assertTrue(sym.type is zbar.Symbol.EAN13)
            self.assertTrue(sym.type is sym.EAN13)
            self.assertEqual(str(sym.type), 'EAN13')

            cfgs = sym.configs
            self.assertTrue(isinstance(cfgs, set))
            for cfg in cfgs:
                self.assertTrue(isinstance(cfg, zbar.EnumItem))
            self.assertEqual(cfgs,
                             set((zbar.Config.ENABLE, zbar.Config.EMIT_CHECK)))

            mods = sym.modifiers
            self.assertTrue(isinstance(mods, set))
            for mod in mods:
                self.assertTrue(isinstance(mod, zbar.EnumItem))
            self.assertEqual(mods, set())

            self.assertTrue(sym.quality > 0)
            self.assertEqual(sym.count, 0)

            # FIXME put a nice QR S-A in here
            comps = sym.components
            self.assertTrue(isinstance(comps, zbar.SymbolSet))
            self.assertEqual(len(comps), 0)
            self.assertTrue(not comps)
            self.assertTrue(tuple(comps) is ())

            data = sym.data
            self.assertEqual(data, '9876543210128')

            loc = sym.location
            self.assertTrue(len(loc) >= 4) # FIXME
            self.assertTrue(isinstance(loc, tuple))
            for pt in loc:
                self.assertTrue(isinstance(pt, tuple))
                self.assertEqual(len(pt), 2)
                # FIXME test values (API currently in flux)

            self.assertTrue(sym.orientation is zbar.Orient.UP)
            self.assertTrue(data is sym.data)
            self.assertTrue(loc is sym.location)

        def set_symbols(syms):
            self.image.symbols = syms
        self.assertRaises(TypeError, set_symbols, ())

        self.scn.recycle(self.image)
        self.assertEqual(len(self.image.symbols), 0)

    def test_scan_crop(self):
        self.image.crop = (0, 71, 114, 9)
        self.assertEqual(self.image.crop, (0, 71, 114, 9))
        n = self.scn.scan(self.image)
        self.assertEqual(n, 0)

        self.image.crop = (12, 24, 90, 12)
        self.assertEqual(self.image.crop, (12, 24, 90, 12))
        n = self.scn.scan(self.image)
        self.assertEqual(n, 0)

        self.image.crop = (9, 24, 96, 12)
        self.assertEqual(self.image.crop, (9, 24, 96, 12))
        self.test_scan()

    def test_scan_again(self):
        self.test_scan()

class TestProcessor(ut.TestCase):
    def setUp(self):
        self.proc = zbar.Processor()

    def tearDown(self):
        del(self.proc)

    def test_type(self):
        self.assertTrue(isinstance(self.proc, zbar.Processor))
        self.assertTrue(callable(self.proc.init))
        self.assertTrue(callable(self.proc.set_config))
        self.assertTrue(callable(self.proc.parse_config))
        self.assertTrue(callable(self.proc.set_data_handler))
        self.assertTrue(callable(self.proc.user_wait))
        self.assertTrue(callable(self.proc.process_one))
        self.assertTrue(callable(self.proc.process_image))

    def test_set_config(self):
        self.proc.set_config()
        self.assertRaises(ValueError, self.proc.set_config, -1)
        self.assertRaises(TypeError, self.proc.set_config, "yomama")
        self.proc.set_config()

    def test_parse_config(self):
        self.proc.parse_config("disable")
        self.assertRaises(ValueError, self.proc.set_config, -1)
        self.proc.set_config()

    def test_request_size(self):
        def set_size(sz):
            self.proc.request_size = sz
        self.assertRaises(ValueError, set_size, (1,))
        self.assertRaises(ValueError, set_size, 1)
        self.proc.request_size = (12, 6)
        self.assertRaises(ValueError, set_size, (1, 2, 3))
        self.assertRaises(ValueError, set_size, "foo")

    def test_processing(self):
        self.proc.init(VIDEO_DEVICE)
        self.assertTrue(self.proc.visible is False)
        self.proc.visible = 1
        self.assertTrue(self.proc.visible is True)
        self.assertEqual(self.proc.user_wait(1.1), 0)

        self.image = zbar.Image(size[0], size[1], 'Y800', data)

        count = [ 0 ]
        def data_handler(proc, image, closure):
            self.assertTrue(proc is self.proc)
            self.assertTrue(image is self.image)
            self.assertEqual(count[0], 0)
            count[0] += 1

            symiter = iter(image)
            self.assertTrue(isinstance(symiter, zbar.SymbolIter))

            syms = tuple(image)
            self.assertEqual(len(syms), 1)
            for sym in syms:
                self.assertTrue(isinstance(sym, zbar.Symbol))
                self.assertTrue(sym.type is zbar.Symbol.EAN13)
                self.assertEqual(sym.data, '9876543210128')
                self.assertTrue(sym.quality > 0)
                self.assertTrue(sym.orientation is zbar.Orient.UP)
            closure[0] += 1

        explicit_closure = [ 0 ]
        self.proc.set_data_handler(data_handler, explicit_closure)

        rc = self.proc.process_image(self.image)
        self.assertEqual(rc, 0)
        self.assertEqual(len(self.image.symbols), 1)
        del(self.image.symbols)
        self.assertEqual(len(self.image.symbols), 0)

        self.assertEqual(self.proc.user_wait(.9), 0)

        self.assertEqual(explicit_closure, [ 1 ])
        self.proc.set_data_handler()

if __name__ == '__main__':
    ut.main()
