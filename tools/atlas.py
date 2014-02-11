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

def transpose(im):
	return im.transpose(Image.ROTATE_90).transpose(Image.FLIP_TOP_BOTTOM)

def npot(n):
	n-=1
	n |= n >> 1
	n |= n >> 2
	n |= n >> 4
	n |= n >> 8
	n |= n >> 16
	return n+1

def render_atlas(images):
	if not type(images) is list:
		images = [ image for image in images ]
	if len(images) == 1:
		w, h = images[0].size
		tw,th = npot(w), npot(h)
		trivial_result = Image.new('RGBA', (tw, th))
		trivial_result.paste(images[0], (0,0))
		if '-preview' in sys.argv:
			trivial_result.show()
		return (trivial_result, [AtlasImage(images[0], False)])

	images = [ AtlasImage(image, True) for image in images ]
	images.sort(key = lambda image: -image.area())
	print '-' * 80
	print 'ATLASSING', len(images), 'IMAGES'
	print '-' * 80

	# make an initial guess of the texture pitch
	total_area = 0
	for image in images:
		total_area += image.area()
	pitch = 128
	while pitch*pitch < total_area:
		pitch += pitch
		if pitch > 2048:
			raise ImagesDontFit(0)

	# brute-force place images
	keep_trying = True
	while keep_trying:
		keep_trying = False
		for i,image in enumerate(images):
			print "Placing %d / %d" % (i+1, len(images))
			w = image.w
			h = image.h
			ww = pitch-w
			length = (pitch - h) * ww
			k = 0
			while k < length:
				x = k % ww
				y = k / ww
				j = 0
				collided = 0
				while j < i and collided == 0:
					collided = images[j].overlaps(x,y,w,h)
					j+=1
				if collided > 0:
					remainder = ww - x
					if collided < remainder:
						k+= collided
					else:
						k += remainder
				else:
					break
			if collided > 0:
				pitch += pitch
				if pitch > 2048:
					raise ImagesDontFit(i)
				keep_trying = True
				break
			else:
				image.x = x
				image.y = y

	# render result
	result = Image.new('RGBA', (pitch,pitch))
	for image in images:
		image.computeUVs(pitch)
		image.renderTo(result, pitch)
	result.save("__proof.png")
	return (result, images)

class AtlasImage:
	def __init__(self, image, check_transpose):
		self.image = image
		w,h = image.size
		self.x = 0
		self.y = 0
		if check_transpose:
			if h > w:
				self.w = h
				self.h = w
				self.transposed = True
			else:
				self.w = w
				self.h = h
				self.transposed = False
		else:
			self.w = w
			self.h = h
			self.transposed = False
		# padding hack - we're going to double-up the pixels on the edge so that
		# tiled images work out OK
		self.w = self.w + 2
		self.h = self.h + 2

	def area(self): 
		return self.w * self.h

	def overlaps(self, x, y, w, h):
		if self.y < y + h and y < self.y + self.h and self.x < x + w and x < self.x + self.w:
			return self.x + self.w - x
		else:
			return 0

	def texels(self):
		return transpose(self.image) if self.transposed else self.image

	def renderTo(self, result, pitch):
		# draw the image itself
		result.paste(self.texels(), (self.x, self.y))

		# draw border-pixels (we double them up so that we don't
		# get minor sampling artifacts at the edge)
		px = result.load()

		if not hasattr(self.image, 'wants_padding'):
			return
		
		def plot(x,y,c):
			if x >= 0 and x < pitch and y >= 0 and y < pitch:
				px[x,y] = c

		# double-up top/bottom
		for x in xrange(self.w):
			plot(self.x+x, self.y-1, px[self.x+x, self.y])
			plot(self.x+x, self.y+self.h, px[self.x+x, self.y+self.h-1])

		# double-up left/right
		for y in xrange(self.h):
			plot(self.x-1, self.y+y, px[self.x, self.y+y])
			plot(self.x+self.w, self.y+y, px[self.x+self.w-1, self.y+y])

		# double-up opposite corners (since the most common case with
		# artifacts is tiling)

		# top-left
		plot(self.x-1, self.y-1, px[self.x, self.y])

		# bottom-left
		plot(self.x-1, self.y+self.h, px[self.x, self.y+self.h-1])

		# top-right
		plot(self.x+self.w, self.y-1, px[self.x+self.w-1, self.y])

		# bottom-right
		plot(self.x+self.w, self.y+self.h, px[self.x+self.w-1, self.y+self.h-1])

	def computeUVs(self, pitch):
		# a tiny bit that the UVs are "snuggled" in, to avoid
		# begin right on the edge
		ep = 0.0001
		
		# remove padding
		self.w = self.w - 2
		self.h = self.h - 2

		# vertex order: top left, bottom left, top right, bottom right
		k = 1.0/float(pitch)
		self.u0, self.v0 = self.x * k + ep, self.y * k + ep
		if self.transposed:
			self.du, self.dv = self.h * k - ep - ep, self.w * k - ep - ep
			self.u1 = self.u0 + self.dv
			self.v1 = self.v0
			self.u2 = self.u0
			self.v2 = self.v0 + self.du
			self.u3 = self.u0 + self.dv
			self.v3 = self.v0 + self.du
		else:
			self.du, self.dv = self.w * k - ep - ep, self.h * k - ep - ep
			self.u1 = self.u0
			self.v1 = self.v0 + self.dv
			self.u2 = self.u0 + self.du
			self.v2 = self.v0
			self.u3 = self.u0 + self.du
			self.v3 = self.v0 + self.dv


class ImagesDontFit:
	def __init__(self, num_placed):
		self.num_placed = num_placed

