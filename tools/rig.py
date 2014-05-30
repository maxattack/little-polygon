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
import json

# Loads a character rig from Spine's JSON Format.
# For our consideration (pending need): 
# * generalize to other kinds of rig formats?
# * procedural interface?

TimelineTranslation = 1
TimelineRotation = 2
TimelineScale = 3

def _name_dict(li):
	return dict( (elem.name, elem) for elem in li )

def _hashes_unique(li):
	return len(li) == len(set(elem.hash for elem in li))

# Convert coordinates from Y = Up to Y = Down
def _fixup_angle(angle): return -angle * math.pi / 180.0
def _fixup_position(x,y): return x,-y

def _make_index(li): 
	for i,elem in enumerate(li): elem.index = i
def _vadd(u, v): return (u[0] + v[0], u[1] + v[1])
def _vmul(u, v): return (u[0] * v[0], u[1] * v[1])

class Rig:
	def __init__(self, path):
		assert os.path.exists(path)
		self.src = open(path, 'r').read()
		self.doc = json.loads(self.src)

		# BONES
		self.bones = [ Bone(self, b) for b in self.doc.get('bones', []) ]
		assert _hashes_unique(self.bones)
		self.name_to_bone = _name_dict(self.bones)
		self.name_to_bone[''] = None
		for i,b in enumerate(self.bones): 
			b.index = i
			b.parent = self.name_to_bone[b.parent_name]
			assert b.parent == None or b.parent.index < b.index

		# SLOTS
		self.slots = [ Slot(self, s) for s in self.doc.get('slots', []) ]
		self.name_to_slot = _name_dict(self.slots)
		_make_index(self.slots)

		# LAYERS
		# Spine calls these "skins" but that makes me think of vertex skinning,
		# so I've renamed them "layers" which is what most other animation systems
		# call this sort of thing.
		self.layers = [ Layer(self, key, val) for key, val in self.doc.get('skins', {}).iteritems() ]
		self.name_to_layer = _name_dict(self.layers)
		assert self.layers[0].name == 'default'
		self.layers[0].hash = 0
		self.defaultLayer = self.layers[1] if len(self.layers) > 1 else self.layers[0]
		assert _hashes_unique(self.layers)

		# Flatten attachments to a list
		self.attachments = [ 
			attachment 
			for layer in self.layers 
			for adict in layer.slot_to_attachments.itervalues() 
			for attachment in adict.itervalues() 
		]
		_make_index(self.attachments)

		# TODO: SORT ATTACHMENTS IN DRAW ORDER (json.loads() munges this) FROM SRC

		# EVENTS
		self.events = [ Event(self, key, val) for key, val in self.doc.get('events', {}).iteritems() ]
		assert _hashes_unique(self.events)
		self.name_to_event = _name_dict(self.events)

		# ANIMATIONS
		self.anims = [ Animation(self, key, val) for key, val in self.doc.get('animations', {}).iteritems() ]
		assert _hashes_unique(self.anims)
		self.name_to_anim = _name_dict(self.anims)
		_make_index(self.anims)

		# Flatten timelines list
		self.timelines = [
			timeline 
			for anim in self.anims 
			for bone_anim in anim.bone_animations
			for timeline in bone_anim.timelines
		]
		_make_index(self.timelines)

class Bone:
	def __init__(self, rig, doc):
		self.rig = rig
		self.name = doc.get('name')
		self.hash = fnv32a(self.name)

		self.x, self.y = _fixup_position(doc.get('x', 0), doc.get('y', 0))
		self.pos = (self.x, self.y)
		self.radians = _fixup_angle(doc.get('rotation', 0))
		self.scaleX = doc.get('scaleX', 1)
		self.scaleY = doc.get('scaleY', 1)
		self.scale = (self.scaleX, self.scaleY)
		self.parent_name = doc.get('parent', '')

def _unpack_color(color):
	return (
		int(color[0:2], 16),
		int(color[2:4], 16),
		int(color[4:6], 16),
		int(color[6:8], 16)
	)

class Slot:
	def __init__(self, rig, doc):
		self.rig = rig
		self.name = doc.get('name')

		self.bone = rig.name_to_bone[doc.get('bone')]
		self.default_attachment_name = doc.get('attachment', '')
		self.default_attachment_hash = fnv32a(self.default_attachment_name)
		self.color = _unpack_color(doc.get('color', 'FFFFFFFF'))


class Layer:
	def __init__ (self, rig, name, doc):
		self.rig = rig
		self.name = name
		self.hash = fnv32a(self.name)

		self.slot_to_attachments = dict()
		for slot_name, attachments_dict in doc.iteritems():
			slot = rig.name_to_slot[slot_name]
			self.slot_to_attachments[slot] = dict(
				(key, Attachment(self, slot, key,val))
				for key,val in attachments_dict.iteritems()
			)


class Attachment:
	def __init__ (self, layer, slot, name, doc):
		self.layer = layer
		self.slot = slot
		self.name = name
		self.hash = fnv32a(self.name)

		# just handle ordinary images for now
		type = doc.get('type', 'region')
		assert type == 'region'

		self.image_id = doc.get('name', name)

		self.x, self.y = _fixup_position(doc.get('x', 0), doc.get('y', 0))
		self.radians = _fixup_angle(doc.get('rotation', 0))
		self.scaleX = doc.get('scaleX', 1)
		self.scaleY = doc.get('scaleY', 1)

		# do we need these?
		self.width = doc.get('width', 0)
		self.height = doc.get('height', 0)

class Event:
	def __init__(self, rig, name, doc):
		self.rig = rig
		self.name = name
		self.hash = fnv32a(self.name)

		self.intValue = doc.get('int', 0)
		self.floatValue = doc.get('float', 0)
		self.stringValue = doc.get('string', '')


class Animation:
	def __init__(self, rig, name, doc): 
		self.rig = rig
		self.name = name
		self.hash = fnv32a(self.name)

		self.bone_animations = [ 
			BoneAnimation(self,rig.name_to_bone[k],v) 
			for k,v in doc.get('bones',{}).iteritems() 
		]
		self.bone_to_animation = dict((anim.bone, anim) for anim in self.bone_animations)

		def unpack_event(e):
			time = e['time']
			event = rig.name_to_event[ e['name'] ]
			intValue = e.get('int', event.intValue)
			floatValue = e.get('float', event.floatValue)
			stringValue = e.get('string', event.stringValue)
			return (time, event, intValue, floatValue, stringValue)
		self.eventanimation = map(unpack_event, doc.get('events', []))

		self.slot_animations = [
			SlotAnimation(self,k,v)
			for k,v in doc.get('slots',{}).iteritems()
		]
		self.slot_toanimation = dict((anim.slot, anim) for anim in self.slot_animations)

		if 'draworder' in doc:
			print 'WARNING: IGNORING DRAWORDER ANIMATIONS (for now)'

		duration = 0
		for anim in self.bone_animations: duration = max(duration, anim.duration)
		for anim in self.slot_animations: duration = max(duration, anim.duration)
		self.duration = duration

class BoneAnimation:
	def __init__(self, anim, bone, doc):
		self.anim = anim
		self.bone = bone

		self.timelines = []
		if 'rotate' in doc: self.timelines.append(Timeline(self, TimelineRotation, doc['rotate']))
		if 'translate' in doc: self.timelines.append(Timeline(self, TimelineTranslation, doc['translate']))
		if 'scale' in doc: self.timelines.append(Timeline(self, TimelineScale, doc['scale']))

		duration = 0
		times = (t for tl in self.timelines for t in tl.times)
		for time in times: duration = max(duration, time)
		self.duration = duration

class Timeline:
	def __init__(self, bone_anim, kind, keys):
		self.bone_anim = bone_anim
		self.bone = bone_anim.bone
		self.kind = kind
		self.times = [ k['time'] for k in keys ]
		# TODO: CURVES		
		if kind == TimelineTranslation:
			self.values = [ comp for k in keys for comp in _vadd(self.bone.pos, _fixup_position(k['x'], k['y'])) ]
		elif kind == TimelineRotation:
			self.values = [ self.bone.radians + _fixup_angle(k['angle']) for k in keys ]
		elif kind == TimelineScale:
			self.values = [ comp for k in keys for comp in _vmul(self.bone.scale, (k['x'], k['y'])) ]

class SlotAnimation:
	def __init__(self, anim, slot_name, doc):
		self.anim = anim
		self.slot = anim.rig.name_to_slot[slot_name]

		self.attachment_keys = [
			(k['time'], k['name'])
			for k in doc.get('attachment', [])
		]

		# TODO: CURVES

		self.color_keys = [
			(k['time'], _unpack_color(k['color']))
			for k in doc.get('color', [])
		]

		duration = 0
		for time,_ in self.attachment_keys: duration = max(duration, time)
		for time,_ in self.color_keys: duration = max(duration, time)
		self.duration = duration

if __name__ == '__main__':
	rig = Rig('../../assets/hero.json')
	print '-' * 80
	for bone in rig.bones:
		if bone.parent:
			print bone.name, '>', bone.parent.name
	print '-' * 80
	for slot in rig.slots:
		print slot.name, slot.bone, slot.color
	print '-' * 80
	for anim in rig.anims:
		print anim.name, anim.duration
	print '-' * 80


