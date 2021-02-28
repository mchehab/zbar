#!/bin/bash
set -e

if [[ $1 = "--help" ]] || [[ $1 = "-h" ]]
then
    echo "this is a script to prepare for zbar_lite build"
    help="Usage: $0 [ZBAR_SOURCE_HOME] [WORKDIR]
          -h, --help        print this help, then exit
          ZBAR_SOURCE_HOME  the zbar source code home dir which should contain dir zbar and include
          WORKDIR           the work dir,default is the dir which this script in
          "
    echo "${help}"
    exit 0
fi

DEFAULT_WORKDIR=$(dirname $(readlink -f "$0"))
WORKDIR=${2:-$DEFAULT_WORKDIR}
ZBAR_SOURCE_HOME=${1:-$WORKDIR}


echo " WORKDIR is $WORKDIR"
echo " ZBAR_SOURCE_HOME is $ZBAR_SOURCE_HOME"

echo "copy zbar source code"
mkdir -p ${WORKDIR}/src/zbar

mkdir -p ${WORKDIR}/src/zbar/video
cp ${ZBAR_SOURCE_HOME}/zbar/video/null.c ${WORKDIR}/src/zbar/video/

cp -r ${ZBAR_SOURCE_HOME}/zbar/decoder ${WORKDIR}/src/zbar/
cp -r ${ZBAR_SOURCE_HOME}/zbar/qrcode ${WORKDIR}/src/zbar/
cp -r ${ZBAR_SOURCE_HOME}/zbar/processor ${WORKDIR}/src/zbar/
cp -r ${ZBAR_SOURCE_HOME}/zbar/window ${WORKDIR}/src/zbar/


cp ${ZBAR_SOURCE_HOME}/include/zbar.h ${WORKDIR}/src/


if [ -f "${ZBAR_SOURCE_HOME}/include/config.h" ]
then
cp ${ZBAR_SOURCE_HOME}/include/config.h ${WORKDIR}/src/
fi

cp ${ZBAR_SOURCE_HOME}/zbar/config.c ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/convert.c ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/debug.h ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/decoder.c ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/decoder.h ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/error.c ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/error.h ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/event.h ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/image.c ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/image.h ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/img_scanner.c ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/img_scanner.h ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/mutex.h ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/qrcode.h ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/refcnt.c ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/refcnt.h ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/scanner.c ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/sqcode.c ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/sqcode.h ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/svg.h ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/symbol.c ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/symbol.h ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/thread.h ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/timer.h ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/video.c ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/video.h ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/window.h ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/window.c ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/processor.c ${WORKDIR}/src/zbar/
cp ${ZBAR_SOURCE_HOME}/zbar/processor.h ${WORKDIR}/src/zbar/

echo "copy zbar python module source code"
mkdir -p ${WORKDIR}/zbar/
cp ${ZBAR_SOURCE_HOME}/python/*.c ${WORKDIR}/zbar/
cp ${ZBAR_SOURCE_HOME}/python/*.h ${WORKDIR}/zbar/

echo "set output encoding to UTF-8"
sed -i 's/"ISO8859-1"/"UTF-8"/g' ${WORKDIR}/src/zbar/qrcode/qrdectxt.c
sed -i 's/"BIG-5"/"UTF-8"/g' ${WORKDIR}/src/zbar/qrcode/qrdectxt.c
sed -i 's/"SJIS"/"UTF-8"/g' ${WORKDIR}/src/zbar/qrcode/qrdectxt.c