#/bin/bash

VERSION=`date +%s`

SOURCE_DIR=`pwd`

DOT_CARDPEEK_DIR="$SOURCE_DIR/dot_cardpeek_dir"

MODIFIED='no'

case "$1"
in
    stat)
    TODO=stat    
    ;;
    update)
    TODO=update
    ;;
    *)
    echo "Usage: $0 [stat|update]"
    exit
    ;;
esac


if [ -d ~/.cardpeek ]; then
  
  cd ~/.cardpeek

  echo "## Retrieving new files:"
  IFS=$'\t\n' 
  for i in `find * \! -path replay/\*`;
  do
     if [ -d "$i" ]; then
        if [ ! -e "$DOT_CARDPEEK_DIR/$i" ]; then
            echo "!! creating dir $DOT_CARDPEEK_DIR/$i"
            mkdir -p "$DOT_CARDPEEK_DIR/$i"
            MODIFIED='yes' 
        fi
     else
        if [ "$i" -nt "$DOT_CARDPEEK_DIR/$i" ]; then
            echo ">> $i is newer than $DOT_CARDPEEK_DIR/$i"
            if [ $TODO = "update" ]; then
                cp -pRv "$i" "$DOT_CARDPEEK_DIR/$i"
                MODIFIED='yes' 
            fi
        elif [ "$i" -ot "$DOT_CARDPEEK_DIR/$i" ]; then
            echo "<< $i is older than $DOT_CARDPEEK_DIR/$i"
            if [ $TODO = "update" ]; then
                cp -pRv "$DOT_CARDPEEK_DIR/$i" "$i"
                MODIFIED='yes' 
            fi
        fi
     fi
  done 

  if [ $TODO = "update" -a $MODIFIED = 'yes' ]; then
     echo "## Removing unecessary files and updating directory modfication time:"
     rm -vf $DOT_CARDPEEK_DIR/replay/* 
     touch $DOT_CARDPEEK_DIR

     echo "## Set version info:"
     echo "   Version ID is now $VERSION"
     echo $VERSION > $DOT_CARDPEEK_DIR/version
     echo $VERSION > ~/.cardpeek/version
     cat > "$SOURCE_DIR/script_version.h" << __EOF
#ifndef SCRIPT_VERSION_H
#define SCRIPT_VERSION_H

#define SCRIPT_VERSION $VERSION

#endif
__EOF
  else
    echo "## No updates needed."
  fi
  echo "## Done"
else
  echo "Error: ~/.cardpeek does not exist. Doing nothing"
fi
