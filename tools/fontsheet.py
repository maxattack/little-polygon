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

from lputil import *
from PIL import ImageFont, ImageDraw
import trim_image

ASCII_BEGIN = 32
ASCII_END = 127
CHARSET = [ chr(ASCII_BEGIN+i) for i in xrange(ASCII_END-ASCII_BEGIN) ]

def image_nontrivial(im):
	p = im.load()
	return any( p[x,y][3]>=8 for x,y in xyrange(*im.size) )


def render_fontsheet(path, sz):
	font = ImageFont.truetype(path, sz)

	# draw glyphs and trim pixels after the fact
	glyphs = [None] * len(CHARSET)
	for i,ch in enumerate(CHARSET):
		canvas = new_image(4*sz, 4*sz)
		draw = ImageDraw.Draw(canvas)
		draw.text((sz,sz), ch, font=font)
		if image_nontrivial(canvas):
			glyphs[i] = trim_image.trim_image(canvas)

	# union all the trim bboxes
	i = 0
	while glyphs[i] is None: i+=1
	x0,y0,x1,y1 = glyphs[i].trim_bbox
	for g in glyphs:
		if g is not None:
			u0,v0,u1,v1 = g.trim_bbox
			x0 = min(x0,u0)
			y0 = min(y0,v0)
			x1 = max(x1,u1)
			y1 = max(y1,v1)
	font_height = y1 - y0 + 1

	# render texture atlas
	result = Image.new('RGBA', (128, 128))
	x, y = (0,0)
	glyph_metrics = [None] * len(CHARSET)
	for i,glyph in enumerate(glyphs):
		w,_ = font.getsize(CHARSET[i])
		if glyph is not None:
			u,v = glyph.trim_offset
			dy = v - y0
			# w,_ = glyph.size
			if x+w > 128:
				x=0
				y+=font_height
			result.paste(glyph, (x,y+dy))
		if x+w > 128:
			x=0
			y+=font_height
		glyph_metrics[i] = (x, y, w)
		x += w

	# cleanup pixels (all white, just modulate alpha)
	p = result.load()
	for y,x in xyrange(128,128):
		r,g,b,a = p[x,y]
		p[x,y] = (255, 255, 255, a)
	return result, font_height, glyph_metrics


if __name__ == '__main__' : 
	# just a test
	texture, height, metrics = render_fontsheet('nokiafc22.ttf', 8)
	cleanup_transparent_pixels(texture)
	texture.show()
