from lputil import *

def load_structured_psd(path):
	im = PSDImage.load(path)
	anchorGroup = next( l.layers for l in im.layers if l.name == u'anchor' )

	def centerX(bbox): return 0.5*(bbox.x1+bbox.x2)
	def centerY(bbox): return 0.5*(bbox.y1+bbox.y2)
	def imageOf(name):
		return _rasterize_psd_layer(im, next( l for l in im.layers if l.name == name ))
	def create_result(layer):
		img,x,y = imageOf(layer.name), centerX(layer.bbox), centerY(layer.bbox)
		

	# unpack anchor image & position
	return [ create_result(layer) for layer in anchorGroup.layers ]



