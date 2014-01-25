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
import assets, json, base64, StringIO

def export_json_assets(assetGroup, outpath):

	def convert_image_to_base64_png(image):
		output = StringIO.StringIO()
		image.save(output, 'PNG')
		result = output.getvalue()
		output.close()
		return "data:image/png;base64," + base64.b64encode(result)

	def export_json_texture(texture):

		return {
			'data': convert_image_to_base64_png(texture.image),
			'images': dict(
				(image.id, export_json_image(texture, image)) 
				for image in texture.images
			)
		}

	def export_json_image(texture, image):
		return {
			'frames': [
				export_json_frame(texture, image, image.atlas_images[i], frame) 
				for i,frame in enumerate(image.frames)
			], 
			'w': image.w,
			'h': image.h,
			'px': image.px,
			'py': image.py,

		}

	def export_json_frame(texture, image, atlas_image, frame):
		trimx, trimy = frame.trim_offset
		fw, fh = frame.size
		tw, th = texture.image.size
		return {
			'w': fw,
			'h': fh,
			'px': image.px-trimx,
			'py': image.py-trimy,
			'u': atlas_image.x / float(tw),
			'v': atlas_image.y / float(th),
			'du': fw / float(tw),
			'dv': fh / float(th),
			'transposed': atlas_image.transposed
		}


	# print '----------'
	# print str(json.dumps({
	# 	'textures': dict((t.id,export_json_texture(t)) for t in assetGroup.textures)
	# }))
	# print '----------'
	
	with open(outpath, 'w') as f:
		f.write(json.dumps({
			'textures': dict((t.id,export_json_texture(t)) for t in assetGroup.textures)
		}))

if __name__ == '__main__':
	export_json_assets(assets.load('assets.xml'), 'assets.json')





