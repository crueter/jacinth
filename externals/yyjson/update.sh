#!/bin/sh -e

find . -mindepth 1 -maxdepth 1 ! -name update.sh -exec rm -rf {} \;

ver=0.13.0
tag=$ver
repo=ibireme/yyjson
artifact=$tag.tar.gz
dir=yyjson-$tag

curl -sSfLO "https://github.com/$repo/archive/$artifact"
tar xf $artifact

cd $dir
rm -rf doc fuzz misc test .git* ./*.swift yyjson.pc.in CMakeLists.txt cmake
mv ./* ..

cd ..

rm -rf $artifact $dir
