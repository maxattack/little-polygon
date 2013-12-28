#!/usr/bin/python
from lputil import *

def trim_image(im):
	w,h = im.size
	p = im.load()

	def is_transparent(px,py):
		return p[px,py][3] < 8

	def is_row_transparent(y):
		return all(is_transparent(x,y) for x in xrange(w))

	def is_col_transparent(x):
		return all(is_transparent(x,y) for y in xrange(h))

	x0, x1 = 0,w
	y0, y1 = 0,h

	while is_col_transparent(x0): x0 += 1
	while is_col_transparent(x1-1): x1 -= 1
	while is_row_transparent(y0): y0 += 1
	while is_row_transparent(y1-1): y1 -= 1

	result = im.crop((x0,y0,x1,y1))
	result.trim_offset = (x0, y0)
	result.trim_bbox = (x0,y0,x1,y1)

	return result

def _test():
	im = open_image(sys.argv[1])
	im = trim_image(im)
	print im.trim_offset
	im.show()

if __name__ == '__main__' and len(sys.argv) == 2:
	_test()
