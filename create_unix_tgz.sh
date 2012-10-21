#! /usr/bin/env bash

# Creates a binary tgz package out of the currently-built tree.
# Ignores its arguments; simply operates within the current directory.
# Output is
#   abendstern-VERSION-KERNEL-ARCH.tgz

# Rebuild
./configure --disable-debug --disable-profile || exit 1
#make clean || exit 1
make $MAKE_OPTIONS || exit 1

VERSION=$(cat version)
KERNEL=$(uname -s)
ARCH=$(uname -m)
BINARIES="abendstern abendsternuxgl14 abendsternuxgl21 abendsternuxgl32"
FILES="abendstern.default.rc data fonts hangar.default images legal"
FILES="$FILES COPYING README shaders tcl version"
LIBRARIES="SDL-1.2 SDL_image-1.2 png12 gmp itcl3.4 tcl8.5"
OPTIONAL_LIBRARIES="drachen jpeg tiff uuid FLAC vorbisenc vorbis ogg webp"
OPTIONAL_LIBRARIES="$OPTIONAL_LIBRARIES directfb-1.2 fusion-1.2 direct-1.2"
OPTIONAL_LIBRARIES="$OPTIONAL_LIBRARIES ts-0.0 jbig drachen"
for opt in $OPTIONAL_LIBRARIES; do
    if ldd src/abendsternuxgl32 | grep $opt >&/dev/null; then
        LIBRARIES="$LIBRARIES $opt"
    fi
done

# amd64 is reported as x86_64, which is a rather silly name
if [ "$ARCH" = "x86_64" ]; then
    ARCH=amd64
fi

OUTNAME="abendstern-$VERSION-$KERNEL-$ARCH"

# Copy files
mkdir $OUTNAME || exit 1
cp -R $FILES $OUTNAME/
mkdir $OUTNAME/bin
for bin in $BINARIES; do
    cp src/$bin $OUTNAME/bin/
done

# Copy libraries
mkdir $OUTNAME/lib
for library in $LIBRARIES; do
    if ldd src/abendsternuxgl32 | grep lib$library.so >/dev/null; then
        name=$(ldd src/abendsternuxgl32 | grep lib$library.so | \
               sed 's/.*=> *//g;s/ *(.*//g')
        if cp $name $OUTNAME/lib; then
            true;
        else
            echo "Could not copy library $library"
            exit 1
        fi
        test -e $OUTNAME/lib/lib$library.so || \
            ln -s $(basename $name) $OUTNAME/lib/lib$library.so
    else
        echo "Could not locate library $library"
        exit 1
    fi
done

# Copy contrib stuff (Tcl libraries and such)
cp -R /usr/share/tcltk/tcl8.5 $OUTNAME/lib/
cp -R /usr/share/tcltk/itcl3.4 $OUTNAME/lib/
cp -R /usr/share/tcltk/tcllib1.14 $OUTNAME/lib/

# Remove obsolete Tcl HTTP package
rm -R $OUTNAME/lib/tcl8.5/http1.0

# Patch autosource.tcl to manually load the http and msgcat libraries since Tcl
# can't find them for whatever reason.
echo ';source' lib/tcl8.5/tcl8/http-2.7.7.tm >>$OUTNAME/tcl/autosource.tcl
echo ';source' lib/tcl8.5/tcl8/msgcat-1.4.4.tm >>$OUTNAME/tcl/autosource.tcl
echo source lib/tcl8.5/tcl8/http-2.7.7.tm | cat - tcl/bkginit.tcl \
    >$OUTNAME/tcl/bkginit.tcl
echo ';source' lib/tcl8.5/tcl8/msgcat-1.4.4.tm >>$OUTNAME/tcl/bkginit.tcl

# Add script to run with the appropriate LD config
cp src/run_from_binary_dist.sh $OUTNAME/abendstern.sh
chmod +x $OUTNAME/abendstern.sh

# Tar up
tar -cf $OUTNAME.tar $OUTNAME || exit 1
# Compress
gzip --best -c $OUTNAME.tar >$OUTNAME.tgz || exit 1
# Clean up
rm -Rf $OUTNAME $OUTNAME.tar
