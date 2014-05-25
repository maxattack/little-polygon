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

import collections, platform, string, struct, sys

Record = collections.namedtuple('Record', ('key', 'format', 'parameters'))
PaddedRecord = collections.namedtuple('PaddedRecord', ('format', 'size', 'alignment'))

SIZE_OF = {
	'x': 1, 'c': 1, 'b': 1, 'B': 1,
	'h': 2, 'H': 2,
	'i': 4, 'I': 4, 'f': 4,
	'q': 8, 'Q': 8,	'd': 8,
	'p': 4, 'P': 8
}

def get_pad(pos, alignment):
	return 0 if pos % alignment == 0 else alignment - (pos % alignment)

def add_padding(fmt):
	max_alignment = 1
	result = ''
	size = 0
	for ch in fmt:
		alignment = SIZE_OF[ch]
		padding = get_pad(size, alignment)
		result += (padding * 'x') + ch
		size += padding + alignment
		if alignment > max_alignment: max_alignment = alignment
	padding = get_pad(size, max_alignment)
	if padding > 0:
		result += (padding * 'x')
		size += padding
	return PaddedRecord(result, size, alignment)
	

def export(records, **kwargs):
	# read kwargs
	byte_order = kwargs.get('byteorder', sys.byteorder)
	byte_order_prefix = '<' if byte_order == 'little' else '>'
	pointer_width = kwargs.get('pointer_width', 64 if platform.architecture()[0] =='64bit' else 32)
	pointer_type = 'p' if pointer_width == 32 else 'P'
	format_pointer_type = 'I' if pointer_width == 32 else 'Q'

	# map keys to indices, update records to replace keys with indexes
	key_to_index = dict((record.key,i) for i,record in enumerate(records))
	if len(key_to_index) < len(records):
		raise Error("bin record IDs not unique")
	def convert_pointer_val(val): return key_to_index[val] if isinstance(val,str) else val
	def convert_pointer_keys_to_indexes(record):
		new_params = tuple(
			convert_pointer_val(val) if record.format[i]=='#' else val
			for i,val in enumerate(record.parameters)
		)
		return Record(record.key, record.format, new_params)
	records = map(convert_pointer_keys_to_indexes, records)
	
	# pad records and note final locations
	padded_records = [ add_padding(format.replace('P', format_pointer_type).replace('#', pointer_type)) for key,format,_ in records ]
	locations = [0]
	location = 0
	for i in xrange(len(records)-1):
		prev = padded_records[i]
		next = padded_records[i+1]
		location += prev.size
		# in-between record padding
		padding = get_pad(location, next.alignment)
		if padding > 0:
			padded_records[i] = PaddedRecord(prev.format + (padding * 'x'), prev.size, prev.alignment)
			location += padding
		locations.append(location)

	# create the actual data
	unpadded_fmt = ''.join(record.format for record in records).replace('x', '')
	padded_fmt = ''.join(record.format for record in padded_records)
	parameters = [ p for r in records for p in r.parameters ]
	for i,ch in enumerate(unpadded_fmt):
		if ch == '#':
			parameters[i] = locations[parameters[i]]
	data = struct.pack(
		byte_order_prefix + padded_fmt.replace(pointer_type, format_pointer_type),
		*parameters
	)

	# create the pointer fixup table
	location = 0
	pointer_locations = []
	for i,ch in enumerate(padded_fmt):
		if ch == pointer_type:
			pointer_locations.append(location)
		location += SIZE_OF[ch]
	pointers = struct.pack(byte_order_prefix + 'I'*len(pointer_locations), *pointer_locations)
	return data, pointers

def _test():
	data, pointers = export(
		[
			Record('bb', (10,11)),
			Record('#i#', (0,42,2)),
			Record('II', (0xdeadbeef, 0x42424242))
		],
		pointer_width=64, 
		byteorder='little'
	)
	with open('data.bin', 'wb') as f : f.write(data)
	with open('pointers.bin', 'wb') as f : f.write(pointers)

if __name__ == '__main__' : _test()


