#!/bin/bash

# TODO this should be a clean build then copy
cp ../build/ali default/airootfs/root/ali
# note: permissions to execute ali are set in profiledef.sh

cp -r ../profiles default/airootfs/root/ali/

# -v verbose
# -r remove working directory when done
# -w set working directory
# -o path for iso
# last arg is profile path
sudo mkarchiso -v -r -w /tmp/archiso-tmp -o iso/ default/
