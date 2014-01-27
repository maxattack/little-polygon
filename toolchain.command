#!/bin/bash
# Little Polygon Setup Script for Mac OS
export LITTLE_POLYGON_DIR="`(cd $(dirname \"$0\"); pwd)`"
export PATH=$PATH:$LITTLE_POLYGON_DIR/tools
export PYTHONPATH=$PYTHONPATH:$LITTLE_POLYGON_DIR/tools
export PS1="[\[\e[1;32m\]little-polygon\[\e[0m\]] \h:\W \u\$ "
export PS2="> "
clear
echo "(>O___O)> IT'S GAME MAKING TIME!"
echo
cd $LITTLE_POLYGON_DIR
exec bash
