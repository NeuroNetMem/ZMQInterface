#!/bin/bash
export GIT_PREFIX=/usr/local
export CONFIG=Release
ln -s ../../../ZMQInterface/ZMQInterface/ ../plugin-GUI/Source/Plugins/ZMQInterface
cd ../plugin-GUI/Builds/Linux/
make   -f Makefile.plugins

