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
import bintools, assets, fontsheet

# D: WARNING potential bug -- 'arrays' assume no trailing-padding!!

ASSET_TYPE_UNDEFINED = 0
ASSET_TYPE_TEXTURE = 1
ASSET_TYPE_IMAGE = 2
ASSET_TYPE_FONT = 3
ASSET_TYPE_SAMPLE = 4
ASSET_TYPE_TILEMAP = 5
ASSET_TYPE_PALETTE = 6
ASSET_TYPE_USERDATA = 7

def export_native_assets(assetGroup, outpath, bpp):
	print '-' * 80
	print 'BUILDING BINARY IMAGE'
	print '-' * 80

	textures = assetGroup.textures
	fonts = assetGroup.fonts
	samples = assetGroup.samples
	tilemaps = assetGroup.tilemaps
	palettes = assetGroup.palettes
	userdata = assetGroup.userdata

	################################################################################
	# 
	# ASSET FILE FORMAT
	# {
	#   asset_hdr_count
	#   asset_hdr[]
	#   {
	#       texture_hdr
	#       {
	#           image_hdr
	#           frame_hdr[]
	#       }[]
	#   }[]
	#   font_hdr[]
	#   tilemap_hdr[]
	#   sample_hdr[]
	#   userdata_hdr[]
	#   compressed_atlas_texture_data[]
	#   compressed_font_texture_data[]
	#   compressed_sample_data[]
	# }
	# 
	################################################################################

	header_count = 1  +\
		len(textures) +\
		len(fonts)    +\
		len(tilemaps) +\
		len(samples)  +\
		len(palettes) +\
		len(userdata)
	for t in textures:
		header_count += 2 * len(t.images) # one record for image, one record for frame[]

	# WRITE HEADERS

	asset_header = []

	for texture in textures:
		asset_header.append((texture.hash, ASSET_TYPE_TEXTURE, texture.id))
		for image in texture.images:
			asset_header.append((image.hash, ASSET_TYPE_IMAGE, image.id))
	for font in fonts:
		asset_header.append((font.hash, ASSET_TYPE_FONT, font.id))
	for tilemap in tilemaps:
		asset_header.append((tilemap.hash, ASSET_TYPE_TILEMAP, tilemap.id))
	for sample in samples:
		asset_header.append((sample.hash, ASSET_TYPE_SAMPLE, sample.id))
	for palette in palettes:
		asset_header.append((palette.hash, ASSET_TYPE_PALETTE, palette.id))
	for data in userdata:
		asset_header.append((data.hash, ASSET_TYPE_USERDATA, data.id))

	# sort by hash, so we can bin-search at runtime
	asset_header.sort(key = lambda tup: tup[0])

	records = [bintools.Record(
		'assertHeaders',
		'II#' * len(asset_header),
		tuple(e for t in asset_header for e in t)
	)]

	for idx,texture in enumerate(textures):
		print "Encoding Texture(%s)" % texture.id
		w,h = texture.image.size
		px = texture.image.load()
		texture_location = len(records)
		# *data
		# Width         : int
		# Height        : int
		# DataLength    : uint
		# TextureHandle : uint (0)
		# Flags         : uint
		records.append(bintools.Record(
			texture.id,
			'#iiIII', 
			("%s_data" % texture.id, w, h, len(texture.data), 0, texture.flags))
		)
		for image in texture.images:
			print "Encoding Image(%s)" % image.id
			nframes = len(image.frames)
			# Texture    : Texture*
			# Width      : int
			# Height     : int
			# PivotX     : int
			# PivotY     : int
			# NumFrames  : int
			records.append(bintools.Record(
				image.id,
				'#iiiii', (texture.id, image.w, image.h, image.px, image.py, nframes)
			))
			format = 'ffffffffiiii' * nframes
			parameters = tuple()
			for frame_index, atlas_image in enumerate(image.atlas_images):
				# Frames[]
				# u0        : float
				# v0        : float
				# u1        : float
				# v1        : float
				# u2        : float
				# v2        : float
				# u3        : float
				# v3        : float
				# PivotX    : int
				# PivotY    : int
				# Width     : int
				# Height    : int
				frame_image = image.frames[frame_index]
				fw, fh = frame_image.size
				trimx, trimy = frame_image.trim_offset
				# u0, v0 = atlas_image.x / float(w), atlas_image.y / float(h)
				# du, dv = fw / float(w), fh / float(h)
				# # vertex order: top left, bottom left, top right, bottom right
				# if atlas_image.transposed:
				# 	u1 = u0 + dv
				# 	v1 = v0
				# 	u2 = u0
				# 	v2 = v0 + du
				# 	u3 = u0 + dv
				# 	v3 = v0 + du
				# else:
				# 	u1 = u0
				# 	v1 = v0 + dv
				# 	u2 = u0 + du
				# 	v2 = v0
				# 	u3 = u0 + du
				# 	v3 = v0 + dv
				parameters += (
					atlas_image.u0, atlas_image.v0, atlas_image.u1, atlas_image.v1, 
					atlas_image.u2, atlas_image.v2, atlas_image.u3, atlas_image.v3,
					image.px-trimx, image.py-trimy,
					fw, fh
				)
			records.append(bintools.Record(
				"%s_frames" % (image.id),
				format, parameters
			))

	for idx,font in enumerate(fonts):
		print "Encoding Font(%s)" % font.id
		# height          : int
		# Glyphs          : int3[GEND-GBEGIN]
		# T:*data
		# T:Width         : int
		# T:Height        : int
		# T:DataLength    : uint
		# T:TextureHandle : uint (0)
		# T:flags         : uint (0)
		records.append(bintools.Record(
			font.id,
			'i' + 'iii'*len(fontsheet.CHARSET) + '#iiIII', \
			(font.height,) + \
			tuple(comp for gmetric in font.metrics for comp in gmetric) + \
			("%s_data" % font.id,) + font.texture.size + (len(font.data), 0, 0)
		))

	for idx,tilemap in enumerate(tilemaps):
		print "Encoding Tilemap(%s)" % tilemap.id
		# *data            : *uint8(0)
		# *MapData         : *uint8
		# TileWidth        : int
		# TileHeight       : int
		# MapWidth         : int
		# MapHeight        : int
		# CompressedLen    : int
		# TA:*data
		# TA:Width         : int
		# TA:Height        : int
		# TA:DataLength    : uint
		# TA:TextureHandle : uint (0)
		# TA:flags         : uint (0)
		mw, mh = tilemap.mapSize
		records.append(bintools.Record(
			tilemap.id,
			'P#IIIII' + '#iiIII',
			(0, "%s_mapData" % tilemap.id, tilemap.tw, tilemap.th, mw, mh, len(tilemap.mapData)) + \
			("%s_atlasData" % tilemap.id,) + tilemap.atlasImg.size + (len(tilemap.atlasData), 0, 0)
		))


	for idx,sample in enumerate(samples):
		print 'Encoding Sample(%s)' % sample.id
		# bufferHandle : uint (0)
		# *data
		# channelCount  : int
		# sampleWidth   : int
		# freq          : int
		# length        : uint
		# compressedLen : uint
		records.append(bintools.Record(
			sample.id,
			'P#iiiII', (
				0,
				"%s_data" % sample.id,
				sample.channel_count, 
				sample.sample_width, 
				sample.freq, 
				sample.uncompressed_size, 
				len(sample.data)
			)
		))

	for palette in palettes:
		print 'Writing Palette (%s)' % palette.id
		# length : int
		# colors : byte[] (RGBARGBA...)
		records.append(bintools.Record(
			palette.id,
			'I' + 'B' * (4*len(palette.colors)),
			(len(palette.colors),) + tuple(c for color in palette.colors for c in color)
		))

	for data in userdata:
		print 'Writing Userdata (%s)' % data.id
		# length  : size_t
		# data    : byte[]
		records += data.records

	# WRITE COMPRESSED DATA BLOCKS

	for texture in textures:
		records.append(bintools.Record(
			"%s_data" % texture.id,
			'B'*len(texture.data), 
			array.array('B', texture.data).tolist()
		))

	for font in fonts:
		records.append(bintools.Record(
			"%s_data" % font.id,
			'B'*len(font.data), 
			array.array('B', font.data).tolist()
		))

	for tilemap in tilemaps:
		records.append(bintools.Record(
			'%s_mapData' % tilemap.id,
			'B'*len(tilemap.mapData), 
			array.array('B', tilemap.mapData).tolist()
		))
		records.append(bintools.Record(
			'%s_atlasData' % tilemap.id,
			'B'*len(tilemap.atlasData), 
			array.array('B', tilemap.atlasData).tolist()
		))

	for sample in samples:
		records.append(bintools.Record(
			'%s_data' % sample.id,
			'B'*len(sample.data), 
			array.array('B', sample.data).tolist()
		))

	data, pointers = bintools.export(records, pointer_width=bpp)

	# WRITE FILE (sizes, payload, pointers)

	with open(outpath, 'wb') as f : 
		f.write(struct.pack('III', bpp, len(data), len(asset_header)))
		f.write(data)
		f.write(pointers)

	# SHOW PROOFS
	# for texture in textures:
	# 	texture.image.show()

################################################################################

if __name__ == '__main__': 
	assert len(sys.argv) >= 2
	input = sys.argv[1]
	output = 'assets.bin' if len(sys.argv) <= 2 else sys.argv[2]
	bpp = 32 if len(sys.argv) <= 3 else int(sys.argv[3])
	export_native_assets(assets.Assets(input), output, bpp)



