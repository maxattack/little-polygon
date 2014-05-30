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
ASSET_TYPE_RIG = 8

def export_native_assets(assetGroup, outpath, bpp):
	print '-' * 80
	print 'BUILDING BINARY IMAGE'
	print '-' * 80

	# WRITE HEADERS

	headers = []
	for texture in assetGroup.textures:
		headers.append((texture.hash, ASSET_TYPE_TEXTURE, texture.id))
		for image in texture.images:
			headers.append((image.hash, ASSET_TYPE_IMAGE, image.id))
	for font in assetGroup.fonts:
		headers.append((font.hash, ASSET_TYPE_FONT, font.id))
	for tilemap in assetGroup.tilemaps:
		headers.append((tilemap.hash, ASSET_TYPE_TILEMAP, tilemap.id))
	for sample in assetGroup.samples:
		headers.append((sample.hash, ASSET_TYPE_SAMPLE, sample.id))
	for palette in assetGroup.palettes:
		headers.append((palette.hash, ASSET_TYPE_PALETTE, palette.id))
	for rig in assetGroup.rigs:
		headers.append((rig.hash, ASSET_TYPE_RIG, rig.id))
	for data in assetGroup.userdata:
		headers.append((data.hash, ASSET_TYPE_USERDATA, data.id))

	# sort by hash, so we can bin-search at runtime
	headers.sort(key = lambda tup: tup[0])
	records = [bintools.Record(
		'assertHeaders',
		'II#' * len(headers),
		tuple(e for t in headers for e in t)
	)]

	for idx,texture in enumerate(assetGroup.textures):

		print "Encoding Texture(%s)" % texture.id
		w,h = texture.image.size
		px = texture.image.load()
		texture_location = len(records)
		# TEXTURE FORMAT
		# *data
		# Width         : int32
		# Height        : int32
		# DataLength    : uint32
		# TextureHandle : uint32 (0)
		# Flags         : uint32
		records.append(bintools.Record(
			texture.id,
			'#iiIII', 
			("%s_data" % texture.id, w, h, len(texture.data), 0, texture.flags))
		)

		for image in texture.images:

			print "Encoding Image(%s)" % image.id
			nframes = len(image.frames)
			frames_name = "%s_frames" % (image.id)
			# IMAGE FORMAT
			# Texture    : Texture*
			# Frames     : Frame*
			# Width      : float32
			# Height     : float32
			# PivotX     : float32
			# PivotY     : float32
			# NumFrames  : int32
			records.append(bintools.Record(
				image.id,
				'##ffffi', (texture.id, frames_name, image.w, image.h, image.px, image.py, nframes)
			))

			# FRAME FORMAT
			# u0        : float32
			# v0        : float32
			# u1        : float32
			# v1        : float32
			# u2        : float32
			# v2        : float32
			# u3        : float32
			# v3        : float32
			# PivotX    : float32
			# PivotY    : float32
			# Width     : float32
			# Height    : float32
			frames_params = tuple()
			for frame_index, atlas_image in enumerate(image.atlas_images):
				frame_image = image.frames[frame_index]
				fw, fh = frame_image.size
				trimx, trimy = frame_image.trim_offset
				frames_params += (
					atlas_image.u0, atlas_image.v0, atlas_image.u1, atlas_image.v1, 
					atlas_image.u2, atlas_image.v2, atlas_image.u3, atlas_image.v3,
					image.px-trimx, image.py-trimy,
					fw, fh
				)
			records.append(bintools.Record(frames_name, 'ffffffffffff'*nframes, frames_params))

	for idx,font in enumerate(assetGroup.fonts):

		print "Encoding Font(%s)" % font.id
		# FONT FORMAT
		# height          : int32
		# Glyphs          : int32[3][GEND-GBEGIN]
		# T:*data
		# T:Width         : int32
		# T:Height        : int32
		# T:DataLength    : uint32
		# T:TextureHandle : uint32 (0)
		# T:flags         : uint32 (0)
		records.append(bintools.Record(
			font.id,
			'i' + 'iii'*len(fontsheet.CHARSET) + '#iiIII', \
			(font.height,) + \
			tuple(comp for gmetric in font.metrics for comp in gmetric) + \
			("%s_data" % font.id,) + font.texture.size + (len(font.data), 0, 0)
		))

	for idx,tilemap in enumerate(assetGroup.tilemaps):

		print "Encoding Tilemap(%s)" % tilemap.id
		# TILEMAP FORMAT
		# *data            : *uint8(0)
		# *MapData         : *uint8
		# TileWidth        : int32
		# TileHeight       : int32
		# MapWidth         : int32
		# MapHeight        : int32
		# CompressedLen    : int32
		# TA:*data
		# TA:Width         : int32
		# TA:Height        : int32
		# TA:DataLength    : uint32
		# TA:TextureHandle : uint32 (0)
		# TA:flags         : uint32 (0)
		mw, mh = tilemap.mapSize
		records.append(bintools.Record(
			tilemap.id,
			'P#IIIII' + '#iiIII',
			(0, "%s_mapData" % tilemap.id, tilemap.tw, tilemap.th, mw, mh, len(tilemap.mapData)) + \
			("%s_atlasData" % tilemap.id,) + tilemap.atlasImg.size + (len(tilemap.atlasData), 0, 0)
		))


	for idx,sample in enumerate(assetGroup.samples):

		print 'Encoding Sample(%s)' % sample.id
		# SAMPLE FORMAT
		# bufferHandle : uint32 (0)
		# *data
		# channelCount  : int32
		# sampleWidth   : int32
		# freq          : int32
		# length        : uint32
		# compressedLen : uint32
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

	for palette in assetGroup.palettes:

		print 'Writing Palette (%s)' % palette.id
		# PALETTE FORMAT
		# length : int32
		# colors : uint8[] (RGBARGBA...)
		records.append(bintools.Record(
			palette.id,
			'I' + 'B' * (4*len(palette.colors)),
			(len(palette.colors),) + tuple(c for color in palette.colors for c in color)
		))

	for rig in assetGroup.rigs:
		def bone_name(bone): return "%s.bone[%d]" % (bone.rig.asset.id, bone.index)
		def slot_name(slot): return "%s.slot[%d]" % (slot.rig.asset.id, slot.index)
		def attach_name(attachment): return "%s.attachment[%d]" % (attachment.slot.rig.asset.id, attachment.index)
		def anim_name(anim): return "%s.anim[%d]" % (anim.rig.asset.id, anim.index)
		def timeline_name(timeline): return "%s.timeline[%d]" % (timeline.bone_anim.anim.rig.asset.id, timeline.index)

		print 'Writing Rig (%s)' % rig.id
		# RIG FORMAT
		# defaultLayer : uint32
		# nbones       : uint32
		# nslots       : uint32
		# nattachments : uint32
		# nanimations  : uint32
		# ntimelines   : uint32
		# bones        : Bone*
		# slots        : Slot*
		# attachments  : Attachment*
		# animations   : Animation*
		# timelines    : Timeline*
		records.append(bintools.Record(
			rig.id, 'IIIIII#####',
			(
				rig.model.defaultLayer.hash, 
				len(rig.model.bones), 
				len(rig.model.slots), 
				len(rig.model.attachments), 
				len(rig.model.anims),
				len(rig.model.timelines),
				bone_name(rig.model.bones[0]), 
				slot_name(rig.model.slots[0]), 
				attach_name(rig.model.attachments[0]),
				anim_name(rig.model.anims[0]),
				timeline_name(rig.model.timelines[0])
			)
		))


		# RIG::BONE FORMAT
		# parentIdx  : uint32
		# hash       : uint32
		# tx         : float32
		# ty         : float32
		# sx         : float32
		# sy         : float32
		# radians    : float32
		for bone in rig.model.bones:
			records.append(bintools.Record(
				bone_name(bone), 'IIfffff',
				(
					bone.parent.index if bone.parent else 0,
					bone.hash,
					bone.x,
					bone.y,
					bone.scaleX,
					bone.scaleY,
					bone.radians
				)
			))

		# RIG::SLOT FORMAT
		# boneIdx       : uint32
		# defaultAttach : uint32
		# r             : uint8
		# g             : uint8
		# b             : uint8
		# a             : uint8
		for slot in rig.model.slots:
			records.append(bintools.Record(
				slot_name(slot), "IIBBBB",
				(slot.bone.index, slot.default_attachment_hash) + slot.color
			))

		# RIG::ATTACHMENT FORMAT
		# slot      : *Slot
		# image     : *Image
		# hash      : uint32
		# layerHash : uint32
		# ux        : float32
		# uy        : float32
		# vx        : float32
		# vy        : float32
		# tx        : float32
		# ty        : float32
		for attachment  in rig.model.attachments:
			s,c = math.sin(attachment.radians), math.cos(attachment.radians)
			records.append(bintools.Record(
				attach_name(attachment), "##IIffffff",
				(
					slot_name(attachment.slot),
					attachment.image.id,
					attachment.hash,
					attachment.layer.hash,
					attachment.scaleX * c, attachment.scaleX * s,
					-attachment.scaleY * s, attachment.scaleY * c,
					attachment.x, attachment.y
				)
			))

		# RIG::ANIMATION FORMAT
		# hash     : uint32
		# duration : float
		for anim in rig.model.anims:
			records.append(bintools.Record(
				anim_name(anim), "If",
				(anim.hash, anim.duration)
			))

		# RIG::TIMELINE FORMAT
		# times      : *float
		# values     : *void
		# nkeyframes : uint32
		# animHash   : uint32
		# targetIdx  : uint32
		# kind       : uint32
		for timeline in rig.model.timelines:
			tlname = timeline_name(timeline)
			records.append(bintools.Record(
				tlname, "##IIII",
				(
					tlname+".times",
					tlname+".values",
					len(timeline.times),
					timeline.bone_anim.anim.hash,
					timeline.bone_anim.bone.index,
					timeline.kind
				)
			))
			records.append(bintools.Record(
				tlname+".times", "f" * len(timeline.times), timeline.times
			))
			records.append(bintools.Record(
				tlname+".values", "f" * len(timeline.values), timeline.values
			))


	for data in assetGroup.userdata:
		print 'Writing assetGroup.Userdata (%s)' % data.id
		records += data.records

	# WRITE COMPRESSED DATA BLOCKS

	for texture in assetGroup.textures:
		records.append(bintools.Record(
			"%s_data" % texture.id,
			'B'*len(texture.data), 
			array.array('B', texture.data).tolist()
		))

	for font in assetGroup.fonts:
		records.append(bintools.Record(
			"%s_data" % font.id,
			'B'*len(font.data), 
			array.array('B', font.data).tolist()
		))

	for tilemap in assetGroup.tilemaps:
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

	for sample in assetGroup.samples:
		records.append(bintools.Record(
			'%s_data' % sample.id,
			'B'*len(sample.data), 
			array.array('B', sample.data).tolist()
		))

	data, pointers = bintools.export(records, pointer_width=bpp)

	# WRITE FILE (sizes, payload, pointers)

	with open(outpath, 'wb') as f : 
		f.write(struct.pack('III', bpp, len(data), len(headers)))
		f.write(data)
		f.write(pointers)

	print '-' * 80
	print 'WROTE ASSETS TO PATH: %s' % outpath
	print '-' * 80

################################################################################

if __name__ == '__main__': 
	assert len(sys.argv) >= 2
	input = sys.argv[1]
	output = 'assets.bin' if len(sys.argv) <= 2 else sys.argv[2]
	bpp = 32 if len(sys.argv) <= 3 else int(sys.argv[3])
	export_native_assets(assets.Assets(input), output, bpp)



