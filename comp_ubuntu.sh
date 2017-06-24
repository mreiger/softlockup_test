#!/bin/bash

export KERNEL_VER=`uname -r`
echo $KERNEL_VER

make -C /usr/src/linux-headers-$KERNEL_VER M=`pwd` modules
