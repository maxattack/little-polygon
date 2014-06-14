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
import atlas, bintools, fontsheet, rig, trim_image, tmx, wave

# TODO
# This file is a bit of a disaster area -- should move some of the deep functions
# related to textures, images, etc into separate portable loaders, and just implement
# "adaptors" here that do validation and put everything in the correct interface for 
# the exporter.
# See e.g. how 'RigAsset' is handled - this should be the model for other objects.


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


		self.userdata = []

		self.textures = [_parse_yaml_texture(self, k, v) for k,v in list_items('texture')]
		self.fonts = [_parse_yaml_font(self, k, v) for k,v in list_items('font')]
		self.samples = [_parse_yaml_sample(self, k, v) for k,v in list_items('sample')]
		self.tilemaps = [_parse_yaml_tilemap(self, k, v) for k,v in list_items('tilemap')]
		self.palettes = [_parse_yaml_palette(self, k, v) for k,v in list_items('palette')]
		self.rigs = [_parse_yaml_rig(self,k,v) for k,v in list_items('rig')]

		all_hashes = \
			[ t.hash for t in self.textures ] + \
			[ i.hash for t in self.textures for i in t.images ] + \
			[ f.hash for f in self.fonts ] +    \
			[ s.hash for s in self.samples ] +  \
			[ t.hash for t in self.tilemaps ] + \
			[ p.hash for p in self.palettes ] + \
			[ r.hash for r in self.rigs ]

		self.hash_set = set(all_hashes)
		assert len(all_hashes) == len(self.hash_set)

	def addUserdata(self, id, format, params, *args):
		ud = Userdata([bintools.Record(id, format, params)] + list(args))
		if hasattr(self, "hash_set"):
			assert not ud.id in self.hash_set
			self.hash_set.add(ud.id)
		self.userdata.append(ud)

################################################################################
# HELPER METHODS

def _set_id(obj, id):
	obj.id = id
	obj.hash = fnv32a(id)

def list_yaml_items(params, name):
	if name in params:
		return params[name].iteritems()
	else:
		return tuple()

def _parse_yaml_path(context, local_path):
	if isinstance(local_path,list):
		return [ _parse_yaml_path(context,p) for p in local_path ]

	if local_path.startswith('/'):
		return local_path

	return os.path.join(context.dir, local_path)

################################################################################
# TEXTURE ASSET

TEXTURE_FLAG_FILTER = 0x01
TEXTURE_FLAG_REPEAT = 0x02
TEXTURE_FLAG_LUM    = 0x04
TEXTURE_FLAG_RGB    = 0x08

def _parse_yaml_texture(context, id, params):
	if isinstance(params, dict):
		images = [
			_parse_yaml_image(context, k, v)
			for k,v in params.iteritems() 
			if k.startswith('image/') or k.startswith('animation/')
		]
		assert len(images) > 0
		compositedImage = _composite_texture(images)
		filter = params.get('filter', '')
	else:
		images = []
		compositedImage = open_image(_parse_yaml_path(context, params))
		filter = 'linear'
	return Texture(id, compositedImage, images, filter)

def _composite_texture(images):
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

class Texture:
	def __init__(self, id, compositedImage, images, filter):
		_set_id(self, id)
		self.image = compositedImage
		cleanup_transparent_pixels(self.image)
		self.images = images

		self.flags = 0
		if filter.lower() == 'linear':
			self.flags |= TEXTURE_FLAG_FILTER
		self.data = zlib.compress(self.image.tostring(), 6)

################################################################################
# IMAGE ASSET

def _parse_yaml_image(context, namespace_id, params):
	t,id = namespace_id.split('/')
	if isinstance(params, dict):
		return _create_image(
			id, 
			_parse_yaml_path(context, params['path']), 
			params.get('pivot', ''), 
			bool(params.get('pad', True)),
			t == 'animation'
		)
	else:
		return _create_image(id, _parse_yaml_path(context, params), 'center', True, False)

def _load_frames(path):
	if isinstance(path,list):
		# list of frames
		frames = map(open_image, path)

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
	elif '%%' in path:
		# we have several images
		def i_to_s(i):
			return '0' + str(i) if i < 10 else str(i)
		count = 0
		while(os.path.exists(path.replace('%%', i_to_s(count)))):
			count += 1
		assert count > 0
		frames = [ open_image(path.replace('%%', i_to_s(i))) for i in xrange(count) ]

	elif '%' in path:
		# we have several images
		count = 0
		while(os.path.exists(path.replace('%', str(count)))):
			count += 1
		assert count > 0
		frames = [ open_image(path.replace('%', str(i))) for i in xrange(count) ]

	else:
		# we have a filmstrip
		im = open_image(path)
		num_frames = int(os.path.splitext(path)[0].split('@')[1]) if '@' in path else 1
		if num_frames == 1:
			frames = [ im ]
		else:
			total_height = im.size[1]
			frame_height = total_height/num_frames
			frames = [
				im.crop((0, i*frame_height, im.size[0], (i+1)*frame_height)) \
				for i in xrange(num_frames)
			]	

	return frames

def _get_pivot(pivot, w, h):
	px, py = 0, 0
	if type(pivot) == list:
		px, py = map(float, pivot[:2])
	else:
		if 'center' in pivot:
			px = w / 2.0
			py = h / 2.0
		if 'top' in pivot:
			py = 0
		elif 'bottom' in pivot:
			py = h
		if 'left' in pivot:
			px = 0
		elif 'right' in pivot:
			px = w
	return px,py

def _trim_frames(frames):
	w, h = 0, 0
	result = []
	for i,frame in enumerate(frames):
		fw, fh = frame.size
		w = max(w, fw)
		h = max(h, fh)
		trimmed = trim_image.trim_image(frame)
		trimmed.frame_number = i
		result.append(trimmed)
	return w,h,result

def _create_image(id, path, pivot, pad, animated):
	frames = open_image_layers(path) if animated else _load_frames(path)
	w, h, frames = _trim_frames(frames)
	px, py = _get_pivot(pivot, w, h)
	return Image(id, w, h, px, py, frames, pad)

class Image:
	def __init__(self, id, w, h, px, py, frames, pad):
		_set_id(self, id)
		self.w, self.h, self.px, self.py = w, h, px, py
		self.frames, self.pad = frames, pad
		for x in frames: 
			x.image_product = self
			if pad:
				x.wants_padding = pad


################################################################################
# FONT ASSET

def _parse_yaml_font(context, id, params):
	return Font(id, _parse_yaml_path(context, params['path']), int(params.get('size', '8')))

class Font:
	def __init__(self, id, path, fontsize):
		_set_id(self, id)
		self.texture, self.height, self.metrics = fontsheet.render_fontsheet(path, fontsize)
		self.data = zlib.compress(self.texture.tostring(), 6)

################################################################################
# SAMPLE ASSET

def _parse_yaml_sample(context, id, params):
	return Sample(id, _parse_yaml_path(context, params))

class Sample:
	def __init__(self, id, path):
		_set_id(self, id)
		self.path = path

		wav = wave.open(path, 'rb')
		self.channel_count, self.sample_width, self.freq, frame_count, _, _ = wav.getparams()
		self.pcm = wav.readframes(frame_count)
		self.uncompressed_size = len(self.pcm)
		self.data = zlib.compress(self.pcm, 6)

################################################################################
# TILEMAP ASSET

def _parse_yaml_tilemap(context, id, params):
	return Tilemap(id, _parse_yaml_path(context, params))

class Tilemap:
	def __init__(self, id, path):
		_set_id(self, id)

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
# PALETTE ASSET

def _parse_yaml_palette(context, id, params):

	if isinstance(params, list):
		# color values are in the text of the asset
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

	else:
		# color values are in a horizontal-image strip
		im = open_image(params)
		px = im.load()
		colors = [ px[0,0] ]
		for x in xrange(im.size[0]):
			next = px[x,0]
			if next != colors[-1]:
				colors.append(next)
		return Palette(id, colors)

class Palette:
	def __init__(self, id, colors):
		_set_id(self, id)
		self.colors = colors

################################################################################
# USERDATA ASSET (Game Script supplies the Formatted Records)

class Userdata:
	def __init__(self, records):
		_set_id(self, records[0].key)
		self.records = records

# Two predefined types of common records: RLE and Zlib-Compressed Buffer Data

def raw_userdata(id, data):
	return bintools.Record(
		id,
		'B' * len(data),
		array.array('B', data).tolist()
	)

def rle_userdata(id, data):
	return bintools.Record(
		id, 
		'I' + ('B' * len(data)),
		[ len(data) ] + array.array('B', data).tolist()
	)

def compressed_userdata(id, data):
	compressed = zlib.compress(data, 6)
	return bintools.Record(
		id,
		'II' + ('B' * len(compressed))
		[len(data), len(compressed)] + array.array('B', compressed).tolist()
	)


################################################################################
# RIG ASSET (based on Spine)

def _parse_yaml_rig(context, id, params):
	return RigAsset(context, id, _parse_yaml_path(context, params))

class RigAsset:
	def __init__(self, context, id, path):
		self.context = context
		_set_id(self, id)
		self.model = rig.Rig(path)
		self.model.asset = self

		# LINK ATTACHMENTS TO IMAGES
		failed = False
		id_to_img = dict( (img.id, img) for tex in context.textures for img in tex.images )
		for attachment in self.model.attachments:
			attachment.image = id_to_img.get(attachment.image_id, None)
			if not attachment.image:
				print "ERROR: IMAGE MISSING FOR RIG (%s | %s)" % (id, attachment.image_id)
				failed = True
		if failed:
			exit(-1)






