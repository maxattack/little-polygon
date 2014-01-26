#!/bin/bash
# SDK Setup Script for Mac OS

export LITTLE_POLYGON_DIR="`(cd $(dirname \"$0\"); pwd)`"
export PYTHONPATH=$PYTHONPATH:$LITTLE_POLYGON_DIR/tools

export PS1="[\[\e[1;32m\]little-polygon\[\e[0m\]] \h:\W \u\$ "
export PS2="> "

clear

cd $LITTLE_POLYGON_DIR
exec bash
