#!/bin/sh

make clean
make TARGET=funkey

mkdir -p opk
cp  main.elf opk/main.elf
cp -r ./filesystem opk/filesystem
cp Formula1.funkey-s.desktop opk/Formula1.funkey-s.desktop

mksquashfs ./opk Formula1.opk -all-root -noappend -no-exports -no-xattrs

rm -r opk