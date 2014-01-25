#!/usr/bin/python

# Little Polygon SDK
# Copyright (C) 2013 Max Kaufmann
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

from PIL import Image
from itertools import product
from glob import glob
from psd_tools import PSDImage
import array, base64, collections, math, os, string, struct, sys, zlib, yaml
import lxml, lxml.etree

def _sanitize_rgba(im):
	return im if im.mode == 'RGBA' else im.convert('RGBA')

def _pad_left_top(im, left, top, sw, sh):
	result = new_image(sw, sh)
	result.paste(im, (left, top))
	return result

def _rasterize_psd_layer(im, layer):
	x,y,_,_ = layer.bbox
	return _pad_left_top(
		_sanitize_rgba(layer.as_PIL()),
		x,y, im.header.width, im.header.height
	)

def open_image(path):

	# if it's a psd maybe we can selectively specify layers?
	# e.g. "path/to/foo.psd{export}"
	return _sanitize_rgba(
		PSDImage.load(path).as_PIL()     \
		if path.lower().endswith('.psd') \
		else Image.open(path)
	)

def open_image_layers(path):
	im = PSDImage.load(path)
	return [_rasterize_psd_layer(im, layer) for layer in im.layers]

def new_image(w,h):
	return Image.new('RGBA', (w,h), (0,0,0,0))

def xyrange(x, y):
	return product(xrange(x), xrange(y))

def npot(n):
	n -= 1
	n |= n >> 1
	n |= n >> 2
	n |= n >> 4
	n |= n >> 8
	n |= n >> 16
	return n+1

def cleanup_transparent_pixels(im):
	px = im.load()
	for x,y in xyrange(*im.size):
		if px[x,y][3] == 0:
			px[x,y] = (0,0,0,0)


