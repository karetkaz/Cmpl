#!/bin/sh

make -C ../cc/src clean
make -C ../gx/src clean
rm ../cc/main ../cc/main.exe
rm ../gx/main ../gx/main.exe

#: Remove QtDevelop generated files
rm -R ../proj/build-cc/*
rm -R ../proj/build-gx/*

#: Remove visual studio generated files
rm ../proj/visual.studio/gxcc.*.suo
rm ../proj/visual.studio/gxcc.*.sdf
#: -R ../proj/visual.studio/Debug
#: -R ../proj/visual.studio/Release
rm -R ../proj/visual.studio/cc/Debug
rm -R ../proj/visual.studio/cc/Release
rm -R ../proj/visual.studio/cc64/Debug
rm -R ../proj/visual.studio/cc64/Release
rm -R ../proj/visual.studio/gx/Debug
rm -R ../proj/visual.studio/gx/Release
rm -R ../proj/visual.studio/lib.gl/Debug
rm -R ../proj/visual.studio/lib.gl/Release
