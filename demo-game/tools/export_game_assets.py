#!/usr/bin/python

from lputil import *
import tmx, assets, export_asset_bin, bintools

# a special loader which also imports game-specific floor data

def loadAssets(path):
	# load asset script
	result = assets.Assets(path)
	
	# load tilemap
	tmap = tmx.TileMap(os.path.join(result.dir, 'test.tmx'))
	w,h = tmap.size

	# find collision-mask tiles
	opaque_ids = (0,)
	opaque_coords = set(
		(tile.x, tile.y)
		for layer in tmap.tilelayers 
		for tile in layer.tiles 
		if tile is not None and tile.id in opaque_ids
	)
	def is_opaque(x,y): return (x,y) in opaque_coords
	bits = [ is_opaque(x,y) for y,x in xyrange(h,w) ]
	nbytes = (len(bits)+7) // 8
	bytes = [0] * nbytes
	for i,bit in enumerate(bits):
		x = i % w
		y = i // w
		if is_opaque(x,y):
			byteIdx = i // 8
			localIdx = i - 8 * byteIdx
			bytes[byteIdx] |= (1<<localIdx)


	# find hero position
	heroObj = next( obj for obj in tmap.objects if obj.type == 'hero' )
	hx,hy = heroObj.position
	hw,hh = heroObj.size
	heroPosition = (hx + 0.5 * hw, hy + hh)
	
	# find kitten position
	kittenObj = next( obj for obj in tmap.objects if obj.type == 'kitten' )
	kx,ky = kittenObj.position
	kw,kh = kittenObj.size
	kittenPosition = (kx + 0.5 * kw, ky + kh)

	# construct world data
	result.addUserdata(
		'world',
		'ffffii#',
		heroPosition + kittenPosition + tmap.size + ('world.maskBytes',),
		assets.raw_userdata('world.maskBytes', bytes)
	)

	return result

if __name__ == '__main__':
	# assert len(sys.argv) >= 2
	# input = sys.argv[1]
	# output = 'assets.bin' if len(sys.argv) <= 2 else sys.argv[2]
	# bpp = 32 if len(sys.argv) <= 3 else int(sys.argv[3])
	export_asset_bin.export_native_assets(loadAssets('assets/assets.yaml'), 'assets/assets.bin', 32)
	



