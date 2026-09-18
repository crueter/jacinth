#!/bin/sh -e

find . -mindepth 1 -maxdepth 1 ! -name update.sh -exec rm -rf {} \;

ver=1.92.0
tag=boost-$ver
repo=boostorg/pfr
artifact=$tag.tar.gz
dir=pfr-$tag

curl -sSfLO "https://github.com/$repo/archive/$artifact"
tar xf $artifact

cd $dir
rm -rf meta .git* build.jam index.html doc example misc test modules/usage*
mv ./* ..
cd ..
rm -rf $artifact $dir
