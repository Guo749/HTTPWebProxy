#!/bin/bash

make clean
git add .
git commit -m "$1"
git push
