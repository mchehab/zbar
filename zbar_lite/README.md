# This package is a part of ZBAR, only supports image related functions. For now , it is only designed for Python.


# How to build and install
**It is recommended for you to run cmd below first to get the right config.h.**

There are two template config.h in the `./config_template` . 

if you did not generate a config.h, we will copy one of them to `./src` according to your OS when setup.
```
cd zbar
autoreconf -vfi
./configure --without-java --without-gtk --without-qt  --without-imagemagick  --disable-video --without-python
```
the ``host`` you should fill with something according to your own compiler.

then run
```
sh preparation.sh
```

before build, you should make sure that your gcc compiler is fit with your OS. 

you can install gcc build env from `https://sourceforge.net/projects/mingw-w64/files/` for windows

*Notice that mingw64 and mingw-w64 is not the same thing.*

It is recommended for you to install setuptools to install and build.

```
pip install setuptools wheel
```

I select `x86_64-posix-seh-rev0` to build my wheel on Windows.

```
python setup.py build -c mingw32
```

to build whl
```
python setup.py bdist_wheel
```

to install
```
python setup.py install
```

## Some errors you could meet:

### cannot find -lmsvcr140

if you build this whl in Windows with `python setup.py build_ext --compiler=mingw32`, 
you may meet an error that `cannot find -lmsvcr140`, as you can see in <https://stackoverflow.com/questions/43873604/where-is-msvcr140-dll-does-it-exist>.

*I fixed it in the setup.py*


# How to use
### *We provide several versions of whl right now. You can try to install via `pip install zbar-lite`.*

```
import zbar
import cv2

img_path='./test.jpg'

# create a reader
scanner = zbar.ImageScanner()

# configure the reader
scanner.parse_config('enable')

# obtain image data
pil = cv2.imread(img_path,cv2.IMREAD_GRAYSCALE)
height, width = pil.shape[:2]
raw = pil.tobytes()

# wrap image data
image = zbar.Image(width, height, 'Y800', raw)

# scan the image for barcodes
scanner.scan(image)

# extract results
for symbol in image:
    # do something useful with results
    print('decoded', symbol.type, 'text', '"%s"' % symbol.data)
    print('type {} text {} location {} quality {}'.format( symbol.type, symbol.data,symbol.location,symbol.quality))

# clean up
del(image)
```
# For more documents you can visit <https://github.com/mchehab/zbar/tree/master/python> 
