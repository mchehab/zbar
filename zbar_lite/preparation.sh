mkdir -p src/zbar/

mkdir -p src/zbar/video
cp ../zbar/video/null.c src/zbar/video/

mkdir -p src/zbar/window
cp ../zbar/window/null.c src/zbar/window/


mkdir -p src/zbar/processor
cp ../zbar/processor/null.c src/zbar/processor/
cp ../zbar/processor/lock.c src/zbar/processor/


cp -r ../zbar/decoder src/zbar/
cp -r ../zbar/qrcode src/zbar/

cp ../python/*.c zbar/
cp ../python/*.h zbar/

cp ../include/zbar.h src/


if [ -f "../include/config.h" ]
then
cp ../include/config.h src/
fi

cp ../zbar/config.c src/zbar/
cp ../zbar/convert.c src/zbar/
cp ../zbar/debug.h src/zbar/
cp ../zbar/decoder.c src/zbar/
cp ../zbar/decoder.h src/zbar/
cp ../zbar/error.c src/zbar/
cp ../zbar/error.h src/zbar/
cp ../zbar/event.h src/zbar/
cp ../zbar/image.c src/zbar/
cp ../zbar/image.h src/zbar/
cp ../zbar/img_scanner.c src/zbar/
cp ../zbar/img_scanner.h src/zbar/
cp ../zbar/mutex.h src/zbar/
cp ../zbar/qrcode.h src/zbar/
cp ../zbar/refcnt.c src/zbar/
cp ../zbar/refcnt.h src/zbar/
cp ../zbar/scanner.c src/zbar/
cp ../zbar/sqcode.c src/zbar/
cp ../zbar/sqcode.h src/zbar/
cp ../zbar/svg.h src/zbar/
cp ../zbar/symbol.c src/zbar/
cp ../zbar/symbol.h src/zbar/
cp ../zbar/thread.h src/zbar/
cp ../zbar/timer.h src/zbar/
cp ../zbar/video.c src/zbar/
cp ../zbar/video.h src/zbar/
cp ../zbar/window.h src/zbar/
cp ../zbar/window.c src/zbar/
cp ../zbar/processor.c src/zbar/
cp ../zbar/processor.h src/zbar/


sed -i 's/#define ZBAR_LITE 0/#define ZBAR_LITE 1/g' ./zbar/zbarmodule.h
sed -i 's/"ISO8859-1"/"UTF-8"/g' ./src/zbar/qrcode/qrdectxt.c
sed -i 's/"BIG-5"/"UTF-8"/g' ./src/zbar/qrcode/qrdectxt.c
sed -i 's/"SJIS"/"UTF-8"/g' ./src/zbar/qrcode/qrdectxt.c
