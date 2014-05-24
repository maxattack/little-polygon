#!/usr/bin/python

from lputil import *
import tmx, assets, export_asset_bin, bintools

if __name__ == '__main__':
	assert len(sys.argv) >= 2
	input = sys.argv[1]
	output = 'assets.bin' if len(sys.argv) <= 2 else sys.argv[2]
	bpp = 32 if len(sys.argv) <= 3 else int(sys.argv[3])
	export_asset_bin.export_native_assets(assets.Assets(input), output, bpp)
	



