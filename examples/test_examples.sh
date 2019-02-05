#!/bin/bash

unset ERR

DIR=`dirname $0`

test()
{
	i=$1
	j=`basename $i`
	CK=`$DIR/../zbarimg/zbarimg $i 2>/dev/null|sha1sum|cut -d" " -f1`
	ORG=`grep $j $DIR/sha1sum|cut -d " " -f1`

	if [ "$CK" != "$ORG" ]; then
		echo "FAILED: $i ($CK instead of $ORG)"
		ERR=1
	fi
}

for i in $DIR/*.png; do
	test $i
done

if [ "$ERR" == "" ]; then
	echo "PASSED."
fi

