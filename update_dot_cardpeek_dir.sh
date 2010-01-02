#/bin/bash

if [ -d ~/.cardpeek ] 
then
  cp -puR ~/.cardpeek/* dot_cardpeek_dir/
  rm dot_cardpeek_dir/logs/*
  touch dot_cardpeek_dir
  echo "updated."
else
  echo "Error: ~/.cardpeek does not exist. Doing nothing"
fi
