#! /bin/bash

BASEDIR=/oss/Abendstern_WGL32
SRVDIR=/programmes/cpp/abendstern/srv/abendstern/package
FILES="apply_update.bat Abendstern.exe abendstern.default.rc data fonts shaders images legal tcl library itcl3.4 tls1.6 tcllib bin"
APP=Abendstern_WGL32

./validation.sh
date -u +%Y%m%d%H%M%S >version
cp version $BASEDIR/version
cp tcl/validation $BASEDIR/validation

rm -f $SRVDIR/manifest
rm -f $SRVDIR/$APP.zip
rm -f $BASEDIR/bin/*.exp $BASEDIR/bin/*.lib

cd $BASEDIR
find -name '*~' | xargs rm -f

for top in $FILES *.dll abendstern.rc version; do
  echo $top
  find $top -type f | grep -v tls16.dll | xargs md5sum >>$SRVDIR/manifest
  cp -R $top $SRVDIR/
done

mkdir /tmp/$APP

for f in $FILES hangar.default *.dll version patchlevel.rc dna; do
  cp -R $f /tmp/$APP
done
pushd /tmp/
zip -r9q $SRVDIR/$APP.zip $APP
popd
rm -R /tmp/$APP
