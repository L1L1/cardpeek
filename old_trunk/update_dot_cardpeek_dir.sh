#/bin/bash

if [ -d ~/.cardpeek ] 
then
  rm -rf dot_cardpeek_dir
  cp -R ~/.cardpeek dot_cardpeek_dir
else
  echo "Error: ~/.cardpeek does not exist. Doing nothing"
fi
