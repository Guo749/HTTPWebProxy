#!/bin/bash

NOPROXYDIR="$1"
PROXYDIR="$2"
numPassed=0
numTests=0
timout=100
tinyServer="vcm-24073.vm.duke.edu"
#tinyServer="localhost"

BASIC_FILES=( "index.html" "home.html" "csapp.c" "tiny.c" "godzilla.jpg" "tiny")

BASIC_HOST=( "http://www.example.com" "http://$tinyServer:15213" "http://$tinyServer:15213" "http://$tinyServer:15213" "http://$tinyServer:15213" "http://$tinyServer:15213" )

function sanityClean {
  rm -rf   $NOPROXYDIR $PROXYDIR
  mkdir -p $NOPROXYDIR $PROXYDIR
  echo "remove and make directory to store fetch result"
}

# curl -v http://www.example.com -o $NOPROXYDIR/index.html --silent
function downloadFiles {
  curl -S -v --max-time $timout $1 -o $2
}

# curl -v --proxy http://localhost:12345 http://www.example.com -o $PROXYDIR/index.html --silent
function downloadFilesProxy {
  echo "$1"
  echo "$2"
  echo "$3"
  curl -S -v --max-time $timout  --proxy $1 $2 -o $3
}

function diffFile {
  diff -q $1 $2
  if [ $? -ne 0 ]
  then
    echo "*** file is different ***"
    echo "*** check the function ***"
    exit 1
  else
    echo "pass the get test"
    ((numPassed++))
  fi
}

function summary {
  echo "------------Summary---------------------"
  echo "test passed $numPassed/$numTests"
  echo "----------------------------------------"
}
### main ###

sanityClean

######################## test basic function of get ###########################################
echo "test case  fetching origin files from one website "
#
for(( i=0;i<${#BASIC_HOST[@]};i++ ))
do
  dest+=$i
  dest+=${BASIC_FILES[$i]}

  downloadFiles ${BASIC_HOST[$i]} "$NOPROXYDIR/$dest"

  if [ $? -ne 0 ]
  then
    echo "cannot fetch ${BASIC_FILES[$i]} directly"
    summary
    exit 1
  fi

  downloadFilesProxy "http://localhost:12345" ${BASIC_HOST[$i]} "$PROXYDIR/$dest"

  if [ $? -ne 0 ]
  then
    echo "cannot fetch ${BASIC_FILES[$i]} with proxy"
    summary
    exit 1
  fi

  ((numTests++))

  diffFile $NOPROXYDIR/$dest $PROXYDIR/$dest
  dest=""
done


summary

####################### test cache ########################################



