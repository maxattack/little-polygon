#!/usr/bin/python

from lputil import *
import tmx, assets, export_asset_bin, bintools

# a special loader which also imports game-specific floor data

def loadAssets(path):
	result = assets.Assets(path)
	tmap = tmx.TileMap(os.path.join(result.dir, 'test.tmx'))
	
	# find opaque tiles
	opaque_ids = (0,)
	opaque_coords = set(
		(tile.x, tile.y)
		for layer in tmap.tilelayers 
		for tile in layer.tiles 
		if tile.id in opaque_ids
	)

	# coalesce left-right into "slabs"
	slabs = set()
	while len(opaque_coords) > 0:
		x0,y = opaque_coords.pop()
		x1 = x0
		while (x0-1,y) in opaque_coords:
			x0 -= 1
			opaque_coords.remove((x0,y))
		while (x1+1,y) in opaque_coords:
			x1 += 1
			opaque_coords.remove((x1,y))
		slabs.add((x0,x1,y))

	# coalesce up-down into colliders
	colliders = set()
	while len(slabs) > 0:
		x0,x1,y0 = slabs.pop()
		y1 = y0
		while (x0,x1,y0-1) in slabs:
			y0 -= 1
			slabs.remove((x0,x1,y0))
		while (x0,x1,y1+1) in slabs:
			y1 += 1
			slabs.remove((x0,x1,y1))
		colliders.add((x0,y0,x1,y1))

	# create data string
	result.addUserdata(
		'colliders', 
		array.array('I', (elem for c in colliders for elem in c)).tostring()
	)

	# find player position
	heroObj = next( obj for obj in tmap.objects if obj.type == 'hero' )
	hx,hy = heroObj.position
	hw,hh = heroObj.size
	slop = 0.1
	result.addUserdata(
		'start',
		struct.pack('ff', hx + 0.5 * hw, hy+hh-slop)
	)

	return result

if __name__ == '__main__':
	assert len(sys.argv) >= 2
	input = sys.argv[1]
	output = 'assets.bin' if len(sys.argv) <= 2 else sys.argv[2]
	bpp = 32 if len(sys.argv) <= 3 else int(sys.argv[3])
	export_asset_bin.export_native_assets(loadAssets(input), output, bpp)
	



