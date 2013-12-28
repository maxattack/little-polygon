#!/usr/bin/python
from lputil import *
import bitarray

def is_transparent(pixel):
	return pixel[3] < 5

def find_sprites(image):
	results = []
	image = image.copy()
	w,h = image.size
	p = image.load()

	# image lookup helpers

	def out_of_bounds(x,y):
		return x < 0 or y < 0 or x >= w or y >= h

	def find_blank_pixel():
		for x,y in xyrange(w,h):
			if not is_transparent(p[x,y]):
				return (x,y)
		return None

	# keep going until image is blank
	anchor = find_blank_pixel()
	
	vmask = bitarray.bitarray(w*h)
	
	def mark_visited(x,y): vmask[y*w+x] = True
	def visited(x,y): return vmask[y*w+x]
	def iter_visited(): return ((i%w, i/w) for i,flag in enumerate(vmask) if flag)

	def visit(x,y):
		if not (out_of_bounds(x,y) or visited(x,y)):
			mark_visited(x,y)
			if not is_transparent(p[x,y]):
				# cardinal neighborhood
				visit(x-1, y)
				visit(x+1, y)
				visit(x,y-1)
				visit(x,y+1)
				# diagonal neighborhood
				visit(x-1, y-1)
				visit(x-1, y+1)
				visit(x+1, y-1)
				visit(x+1, y+1)


	while anchor is not None:
		vmask.setall(False)
		ax, ay = anchor
		visit(ax, ay)
		xmin, xmax, ymin, ymax = ax, ax, ay, ay

		for x,y in iter_visited():
			if not is_transparent(p[x,y]):
				xmin = min(xmin, x)
				xmax = max(xmax, x)
				ymin = min(ymin, y)
				ymax = max(ymax, y)

		result = new_image(xmax-xmin+1, ymax-ymin+1)
		rp = result.load()
		for x,y in iter_visited():
			if not is_transparent(p[x,y]):
				rp[x-xmin, y-ymin] = p[x,y]
				p[x,y] = (0,0,0,0)

		results.append(result)
		anchor = find_blank_pixel()

	return results

def stripify(images):
	wmax, hmax = 0, 0
	for image in images:
		w,h = image.size
		wmax,hmax = max(wmax, w), max(hmax, h)
	result = new_image(wmax, hmax*len(images))
	for i,image in enumerate(images):
		# align all the results to the bottom-center
		w,h = image.size
		padx = (wmax-w)>>1
		pady = (hmax-h)
		result.paste(image, (padx,i*hmax+pady))
	return result


if __name__ == '__main__':
	assert len(sys.argv) > 1
	images = find_sprites(open_image(sys.argv[1]))
	if len(sys.argv) == 2:
		stripify(images).show()
	else:
		stripify(images).save(sys.argv[2])

