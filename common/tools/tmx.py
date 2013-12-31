#!/usr/bin/python
from lputil import *
import zlib

# SOURCE OBJECTS

def unpackProperties(node):
	node = node.find('properties')
	return dict() if node is None else dict( 
		(p.get('name'), p.get('value')) 
		for p in node.iterfind('property') 
	)

class TileSet:
	def __init__(self, tilemap, node):
		self.tilemap = tilemap
		self.name = node.get('name')
		self.gid = int(node.get('firstgid'))
		imageNode = node.find('image')
		baseDir = os.path.split(tilemap.path)[0]
		atlasImage = open_image(os.path.join(baseDir, imageNode.get('source')))
		
		# validate some basic assumptions
		desiredSize = (int(imageNode.get('width')), int(imageNode.get('height')))
		assert desiredSize == atlasImage.size
		desiredTileSize = (int(node.get('tilewidth')), int(node.get('tileheight')))
		assert desiredTileSize == tilemap.tilesize

		pw, ph = desiredTileSize
		self.tw, self.th = desiredSize[0]/pw, desiredSize[1]/ph
		self.tilecount = self.tw * self.th
		self.images = [ 
			atlasImage.crop((pw*tx, ph*ty, pw*(tx+1), ph*(ty+1)))
			for ty,tx in xyrange(self.th, self.tw) 
		]

class Tile:
	def __init__(self, layer, lid, gid):
		self.layer = layer
		self.lid = lid
		pitch = layer.tilemap.size[0]
		self.x, self.y = lid % pitch, lid / pitch
		self.gid = gid
		self.tileset, self.id = layer.tilemap.lookupGid(gid)

	def image(self):
		return self.tileset.images[self.id]

class TileLayer:
	def __init__(self, tilemap, node):
		self.tilemap = tilemap
		self.name = node.get('name')
		self.properties = unpackProperties(node)
		
		# validate some basic assumptions
		w,h = int(node.get('width')), int(node.get('height'))
		assert (w,h) == tilemap.size
		dataNode = node.find('data')

		encoding = dataNode.get('encoding')
		assert encoding in ('csv', 'base64')
		
		if encoding == 'csv':
			payload = [int(token) for token in map(string.strip, dataNode.text.split(',')) if token]
		else:
			# warnink!  assumes we're little-endian (hope dat doesn't matter...)
			payload = array.array('I', base64.b64decode(dataNode.text))
			compression = dataNode.get('compression', '')
			# if compression == 'zlib':
			# 	payload = zlib.decompress(payload)
			# else:
			assert compression == ''

		assert len(payload) == w*h
		self.tiles = [None] * len(payload)
		for i,gid in enumerate(payload):
			if gid: self.tiles[i] = Tile(self, i, gid)

def r2i(x): return int(round(x))

class TileObject:
	def __init__(self, tilemap, node):
		self.tilemap = tilemap
		self.name = node.get('name', '')
		self.type = node.get('type', '').lower().strip()
		tw, th = tilemap.tilesize
		self.position = ( float(node.get('x'))/tw, float(node.get('y'))/th )
		self.size = ( float(node.get('width'))/tw, float(node.get('height'))/th )

class TileMap:
	def __init__(self, path):
		self.path = path
		doc = lxml.etree.parse(path)
		mapnode = doc.getroot()
		self.properties = unpackProperties(mapnode)
		self.size = (int(mapnode.get('width')), int(mapnode.get('height')))
		self.tilesize = (int(mapnode.get('tilewidth')), int(mapnode.get('tileheight')))
		self.tilesets = [TileSet(self, ts) for ts in doc.iterfind('tileset')]
		self.tilelayers = [TileLayer(self, l) for l in doc.iterfind('layer')]
		self.objects = [ TileObject(self, obj) for obj in doc.iterfind('objectgroup/object') ]

	def pixelSize(self):
		w,h = self.size
		tw,th = self.tilesize
		return w*tw, h*th

	def lookupGid(self, gid):
		for tileset in self.tilesets:
			if gid >= tileset.gid and gid < tileset.gid + tileset.tilecount:
				return tileset, gid-tileset.gid
		return None, gid

# PRODUCT OBJECTS

def renderLayer(layer):
	result = new_image( *layer.tilemap.pixelSize() )
	tw, th = layer.tilemap.tilesize
	for tile in layer.tiles:
		if tile is None: continue
		result.paste(tile.image(), (tw*tile.x, th*tile.y), tile.image())
	return result;

def renderMap(tilemap):
	result = new_image( *tilemap.pixelSize() )
	tw, th = tilemap.tilesize
	for layer in tilemap.tilelayers:
		for tile in layer.tiles:
			if tile is None: continue
			result.paste(tile.image(), (tw*tile.x, th*tile.y), tile.image())
	return result

def cutUpImage(im, tw, th):
	w, h = im.size
	countX, countY = w / tw, h / th
	return [
		im.crop((x*tw, y*th, (x+1)*tw, (y+1)*th))
		for y,x in xyrange(countY, countX)
	]


def dedupTiles(sourceTiles):
	atlasTiles = []
	sourceIdxToAtlasIdx = []
	for tile in sourceTiles:
		idx = -1
		for i,atlasTile in enumerate(atlasTiles):
			if tile.tostring() == atlasTile.tostring():
				idx = i
				break
		if idx == -1:
			idx = len(atlasTiles)
			atlasTiles.append(tile)
		sourceIdxToAtlasIdx.append(idx)
	return atlasTiles, sourceIdxToAtlasIdx


def renderTilemapTextures(img, tilesize):
	sourceTiles = cutUpImage(img, tilesize, tilesize)
	atlasTiles, sourceIdxToAtlasIdx = dedupTiles(sourceTiles)

	atlasDim = 4
	while atlasDim * atlasDim < len(atlasTiles):
		atlasDim += 1

	atlasImageW = npot(atlasDim * tilesize)
	assert atlasImageW < 2048
	atlasImg = new_image(atlasImageW, atlasImageW)
	for i,atlasTile in enumerate(atlasTiles):
		x, y = i%atlasDim, i/atlasDim
		atlasImg.paste(atlasTile, (x*tilesize, y*tilesize))

	def listCoords():
		for idx in sourceIdxToAtlasIdx:
			yield idx % atlasDim
			yield idx / atlasDim

	pw, ph = img.size
	tw, th = pw/tilesize, ph/tilesize
	return atlasImg, array.array('B', listCoords()), (tw, th)

def initializeTmxWithImage(path, outW, outH):
	basepath = os.path.splitext(path)[0]
	name = os.path.split(basepath)[1]
	assert name.endswith('px')
	tileSize = 16
	idx = name.rfind('_')
	if idx >= 0:
		tileSize = int(name[idx+1:-2])
		name = name[:idx]
	basepath = os.path.join(os.path.split(basepath)[0], name)

	# dice up tiles
	im = open_image(path)
	tw, th = tileSize, tileSize
	w, h = im.size
	countX, countY = w / tw, h / th
	outImg = im.crop((0,0,countX*tw, countY*th))
	outImg.save(name + '.png')

	# write ex-em-ell
	with open(basepath + '.tmx', 'w') as out:
		tabs = 0
		def write(msg):
			out.write('\t'*tabs)
			out.write(msg)
			out.write('\n')
		write('<?xml version="1.0" encoding="UTF-8"?>')
		write('<map version="1.0" orientation="orthogonal" width="%d" height="%d" tilewidth="%d" tileheight="%d">' % (
			outW, outH, tw, th
		))
		tabs += 1
		write('<tileset firstgid="1" name="%s" tilewidth="%d" tileheight="%d">' % (name, tw, th))
		tabs += 1
		write('<image source="%s.png" width="%d" height="%d" />' % (name, outImg.size[0], outImg.size[1]))
		tabs -= 1
		write('</tileset>')
		write('<layer name="Empty" width="%d" height="%d">' % (outW, outH))
		tabs += 1
		write('<data encoding="base64">')
		tabs += 1
		write(base64.b64encode(array.array('I', [0]*(outW*outH)).tostring()))
		tabs -= 1
		write('</data>')
		tabs -= 1
		write('</layer>')
		tabs -= 1
		write('</map>')



# TESTING

if __name__ == '__main__':
	if len(sys.argv) == 1: 
		foo(10)
		exit(0)
	
	keyArg = sys.argv[1]
	if keyArg.startswith('-'):
		if keyArg == '-init':
			path = sys.argv[2]
			if len(sys.argv) == 3:
				outW, outH = 16, 16
			elif len(sys.argv) == 4:
				outW = int(sys.argv[3])
				outH = outW
			elif len(sys.argv) == 5:
				outW = int(sys.argv[3])
				outH = int(sys.argv[4])
			initializeTmxWithImage(sys.argv[2], outW, outH)
	else:
		tilemap = TileMap(keyArg)
		composited = renderMap(tilemap)
		tw,th = tilemap.tilesize
		assert tw == th
		atlasImg, mapImg, mapSz = renderTilemapTextures(composited, tw)
		atlasImg.show()
		mapImg.show()
