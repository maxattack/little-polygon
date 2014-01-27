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
import atlas, bintools, fontsheet, trim_image, tmx, wave

################################################################################
# HELPER METHODS
################################################################################

def fnv32a( str ):
    hval = 0x811c9dc5
    fnv_32_prime = 0x01000193
    uint32_max = 2 ** 32
    for s in str:
        hval = hval ^ ord(s)
        hval = (hval * fnv_32_prime) % uint32_max
    return hval

def set_id(obj, id):
	obj.id = id
	obj.hash = fnv32a(id)

################################################################################
# CONSTANTS
################################################################################

TEXTURE_FLAG_FILTER = 0x01
TEXTURE_FLAG_REPEAT = 0x02
TEXTURE_FLAG_LUM    = 0x04
TEXTURE_FLAG_RGB    = 0x08

################################################################################
# DATA MODELS
################################################################################

class Texture:
	def __init__(self, id, compositedImage, images, flags):
		set_id(self, id)
		self.image = compositedImage
		cleanup_transparent_pixels(self.image)
		self.images = images
		self.flags = flags
		self.data = zlib.compress(self.image.tostring(), 6)

################################################################################

class Image:
	def __init__(self, id, path, pivot, num_frames):
		set_id(self, id)

		if path.lower().endswith('.psd[]'):
			# photoshop animation
			frames = open_image_layers(path[:-2] )
		elif path.lower().endswith('.gif'):
			# animated gif
			im = open_image(path)
			curr_frame = 0
			frames = []
			while 1:
				try:
					im.seek(curr_frame)					
					# hack to get around broken palettes in PIL
					if curr_frame == 0:
						palette = im.getpalette()
					else:
						im.putpalette(palette)					
					frames.append(im.convert('RGBA'))
					curr_frame += 1
				except EOFError:
					break
		elif '##' in path:
			# we have several images
			def i_to_s(i):
				return '0' + str(i) if i < 10 else str(i)
			count = 0
			while(os.path.exists(path.replace('##', i_to_s(count)))):
				count += 1
			assert count > 0
			frames = [ open_image(path.replace('##', i_to_s(i))) for i in xrange(count) ]

		elif '#' in path:
			# we have several images
			count = 0
			while(os.path.exists(path.replace('#', str(count)))):
				count += 1
			assert count > 0
			frames = [ open_image(path.replace('#', str(i))) for i in xrange(count) ]

		else:
			# we have a filmstrip
			im = open_image(path)
			if num_frames == 1:
				frames = [ im ]
			else:
				total_height = im.size[1]
				frame_height = total_height/num_frames
				frames = [
					im.crop((0, i*frame_height, im.size[0], (i+1)*frame_height)) \
					for i in xrange(num_frames)
				]		

		self.frames = frames

		# trim frames and get logical bounds
		self.w, self.h = 0, 0
		for i,frame in enumerate(self.frames):
			fw, fh = frame.size
			self.w = max(self.w, fw)
			self.h = max(self.h, fh)
			trimmed = trim_image.trim_image(frame)
			trimmed.image_product = self
			trimmed.frame_number = i
			self.frames[i] = trimmed

		# compute pivot
		self.px, self.py = 0, 0
		if ',' in pivot:
			self.px, self.py = map(int, pivot.split(','))
		else:
			if 'center' in pivot:
				self.px = self.w >> 1
				self.py = self.h >> 1
			if 'top' in pivot:
				self.py = 0
			elif 'bottom' in pivot:
				self.py = self.h
			if 'left' in pivot:
				self.px = 0
			elif 'right' in pivot:
				self.px = self.w

################################################################################

class Font:
	def __init__(self, id, path, fontsize):
		set_id(self, id)
		self.texture, self.height, self.metrics = fontsheet.render_fontsheet(path, fontsize)
		self.data = zlib.compress(self.texture.tostring(), 6)

################################################################################

class Sample:
	def __init__(self, id, path):
		set_id(self, id)
		self.path = path

		wav = wave.open(path, 'rb')
		self.channel_count, self.sample_width, self.freq, frame_count, _, _ = wav.getparams()
		self.pcm = wav.readframes(frame_count)
		self.uncompressed_size = len(self.pcm)
		self.data = zlib.compress(self.pcm, 6)

################################################################################

class Tilemap:
	def __init__(self, id, path):
		set_id(self, id)

		print '-' * 80
		print 'RENDERING TILEMAP'
		print '-' * 80
		if path.endswith('.tmx'):

			tilemap = tmx.TileMap(path)
			compositedImage = tmx.renderMap(tilemap)
			self.tw,self.th = tilemap.tilesize
			
			# should differentiate between optimized and raw
			# tile atlasses

			# also, how do we handle layers?

		else:

			assert False
			# compositedImage = open_image(path)
			# self.tw = int(node.get("tw", "16"))
			# self.th = int(node.get("th", "16"))
			# if it's a PSD, should we handle layers?

		print "imgsize =%d,%d" % compositedImage.size

		self.atlasImg, mapArray, self.mapSize = \
			tmx.renderTilemapTextures(compositedImage, self.tw)

		cleanup_transparent_pixels(self.atlasImg)
		self.atlasData = zlib.compress(self.atlasImg.tostring(), 6)
		self.mapData = zlib.compress(mapArray.tostring(), 6)		

################################################################################

class Palette:
	def __init__(self, id, colors):
		set_id(self, id)
		self.colors = colors

################################################################################

class Userdata:
	def __init__(self, id, data):
		set_id(self, id)
		self.data = data

################################################################################
# DATA CONTAINER
################################################################################

class Assets:
	def __init__(self, path):
		with open(path, 'r') as f: 
			self.dir = os.path.split(path)[0]
			self.doc = yaml.load(f.read())

		def list_items(name): 
			prefix = name + '/'
			for k,v in self.doc.iteritems():
				if k.startswith(prefix):
					yield k[len(prefix):], v


		self.textures = [load_yaml_texture(self, k, v) for k,v in list_items('texture')]
		self.fonts = [load_yaml_font(self, k, v) for k,v in list_items('font')]
		self.samples = [load_yaml_sample(self, k, v) for k,v in list_items('sample')]
		self.tilemaps = [load_yaml_tilemap(self, k, v) for k,v in list_items('tilemap')]
		self.palettes = [load_yaml_palette(self, k, v) for k,v in list_items('palette')]
		self.userdata = []

		all_hashes = \
			[ t.hash for t in self.textures ] + \
			[ i.hash for t in self.textures for i in t.images ] + \
			[ f.hash for f in self.fonts ] +    \
			[ s.hash for s in self.samples ] +  \
			[ t.hash for t in self.tilemaps ] + \
			[ p.hash for p in self.palettes ] + \
			[ d.hash for d in self.userdata ]

		self.hash_set = set(all_hashes)
		assert len(all_hashes) == len(self.hash_set)

	def addUserdata(self, id, data):
		assert not id in self.hash_set
		self.hash_set.add(id)
		self.userdata.append(Userdata(id, data))

	def addCompressedUserdata(self, id, data):
		self.addUserdata(id, zlib.compress(data, 6))

################################################################################
# YAML LOADING METHODS
################################################################################

LoaderContext = collections.namedtuple('LoaderContext', ('dir', 'doc'))

def composite_texture(images):
	result, regions = atlas.render_atlas( 
		frame for image in images for frame in image.frames 
	)
	for image in images:
		image.atlas_images = [None] * len(image.frames)
	for atlas_image in regions:
		image = atlas_image.image.image_product
		frame_number = atlas_image.image.frame_number
		image.atlas_images[frame_number] = atlas_image
	return result

def list_yaml_items(params, name):
	if name in params:
		return params[name].iteritems()
	else:
		return tuple()

def load_yaml_path(context, local_path):
	if local_path.startswith('/'):
		result = local_path
	else:
		result = os.path.join(context.dir, local_path)
	return result

def load_yaml_texture(context, id, params):
	if isinstance(params, dict):
		images = [load_yaml_image(context, k, v)for k,v in params.iteritems()]
		assert len(images) > 0
		compositedImage = composite_texture(images)
	else:
		images = []
		compositedImage = open_image(load_yaml_path(context, params))
	return Texture(id, compositedImage, images, 0)

def load_yaml_palette(context, id, params):
	# gift ideas - palette images?
	def sToC(s):
		x = int(s,16)
		return 0 if x < 0 else 255 if x > 255 else x
	def toRGBA(s):
		if s.startswith('rgb(') and s.endswith(')'):
			s = s[4:-1]
			assert len(s) == 6
			return (
				sToC(s[0:2]),
				sToC(s[2:4]),
				sToC(s[4:6]),
				255
			)
		elif s.startswith('rgba(') and s.endswith(')'):
			s = s[5:-1]
			assert len(s) == 8
			return (
				sToC(s[0:2]),
				sToC(s[2:4]),
				sToC(s[4:6]),
				sToC(s[6:8])
			)
		else:
			raise "Bad Color Format: %s" % s
	return Palette(id, map(toRGBA, params))

def load_yaml_image(context, namespace_id, params):
	t,id = namespace_id.split('/')

	# dumb hack for now
	def fixup_path(path):
		if t == 'animation' and path.endswith('.psd'):
			return path + '[]'
		else:
			return path

	if isinstance(params, dict):
		return Image(
			id, 
			fixup_path(load_yaml_path(context, params['path'])), 
			params.get('pivot', ''), 
			int(params.get('frames', '1'))
		)
	else:
		return Image(id, fixup_path(load_yaml_path(context, params)), '', 1)

def load_yaml_font(context, id, params):
	return Font(id, load_yaml_path(context, params['path']), int(params.get('size', '8')))

def load_yaml_sample(context, id, params):
	return Sample(id, load_yaml_path(context, params))

def load_yaml_tilemap(context, id, params):
	return Tilemap(id, load_yaml_path(context, params))

