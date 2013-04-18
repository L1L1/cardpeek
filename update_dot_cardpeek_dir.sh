#/bin/bash

VERSION=`date +%s`

if [ -d ~/.cardpeek ] 
then
  echo "** Retrieving new files **"
  cp -pRv ~/.cardpeek/* dot_cardpeek_dir
  echo "** Removing unecessary files **"
  rm -vf dot_cardpeek_dir/logs/* 
  touch dot_cardpeek_dir
  echo "** Set version info **"
  echo "Version ID is now $VERSION"
  echo $VERSION > dot_cardpeek_dir/version
  echo $VERSION > ~/.cardpeek/version
  cat > script_version.h << __EOF
#ifndef SCRIPT_VERSION_H
#define SCRIPT_VERSION_H

#define SCRIPT_VERSION $VERSION

#endif
__EOF
  echo "** updated **"
else
  echo "Error: ~/.cardpeek does not exist. Doing nothing"
fi
