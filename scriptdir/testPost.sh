#!/bin/bash
# use to test if post is OK

noproxy="postnoproxy.txt"
proxy="postproxy.txt"
noargproxy="noargproxy.txt"
noargnoproxy="noargnoproxy.txt"
numTests=0
numPassed=0

function diffFile {
  ((numTests++))
  diff -q "$1" "$2"
  if [ $? -ne 0 ]
  then
    echo "*** file is different ***"
    echo "*** check the function ***"
    exit 1
  else
    ((numPassed++))
  fi
}

# sanity clean
rm -f postnoproxy.txt postproxy.txt

curl -X POST -d 'a=b' http://vcm-24073.vm.duke.edu:8000/api/register -o $noproxy

if [ $? -ne 0 ]
then
  echo "***fetch error from remote***"
  exit 1
fi

curl -X POST -d 'a=b' http://vcm-24073.vm.duke.edu:8000/api/register -o $proxy --proxy http://localhost:12345

if [ $? -ne 0 ]
then
  echo "***fetch error from remote using proxy***"
  exit 1
fi

diffFile $proxy $noproxy


# test no argument
curl -X POST  http://vcm-24073.vm.duke.edu:8000/api/register -o $noargproxy --proxy http://localhost:12345
if [ $? -ne 0 ]
then
  echo "***fetch error from remote using proxy***"
  exit 1
fi

echo "========pass the post test========"
