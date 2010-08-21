#! /bin/sh
$EXTRACTRC *.ui >> rc.cpp
$XGETTEXT *.h *.cpp -o $podir/ksnapshot.pot
