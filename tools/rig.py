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


def _name_dict(li):
	return dict( (elem.name, elem) for elem in li )

def _hashes_unique(li):
	return len(li) == len(set(elem.hash for elem in li))

class Rig:
	def __init__(self, path):
		assert os.path.exists(path)
		self.src = open(path, 'r').read()
		self.doc = json.loads(self.src)

		self.bones = [ Bone(self, b) for b in self.doc.get('bones', []) ]
		assert _hashes_unique(self.bones)
		self.name_to_bone = _name_dict(self.bones)
		self.name_to_bone[''] = None
		for b in self.bones: 
			b.parent = self.name_to_bone[b.parent_name]

		self.slots = [ Slot(self, s) for s in self.doc.get('slots', []) ]
		self.name_to_slot = _name_dict(self.slots)

		self.skins = [ Skin(self, key, val) for key, val in self.doc.get('skins', {}).iteritems() ]
		self.name_to_skin = _name_dict(self.skins)
		assert self.skins[0].name == 'default'

		self.attachments = [ 
			attachment 
			for skin in self.skins 
			for adict in skin.slot_to_attachments.itervalues() 
			for attachment in adict.itervalues() 
		]

		# TODO: SORT ATTACHMENTS IN DRAW ORDER (json.loads() munges this) FROM SRC

		self.events = [ Event(self, key, val) for key, val in self.doc.get('events', {}).iteritems() ]
		assert _hashes_unique(self.events)
		self.name_to_event = _name_dict(self.events)

		self.anims = [ Animation(self, key, val) for key, val in self.doc.get('animations', {}).iteritems() ]
		assert _hashes_unique(self.anims)
		self.name_to_anim = _name_dict(self.anims)



class Bone:
	def __init__(self, rig, doc):
		self.rig = rig
		self.name = doc.get('name')
		self.hash = fnv32a(self.name)

		self.x = doc.get('x', 0)
		self.y = doc.get('y', 0)
		self.rotation = doc.get('rotation', 0)
		self.scaleX = doc.get('scaleX', 1)
		self.scaleY = doc.get('scaleY', 1)
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
		self.setup_attachment_name = doc.get('attachment', '')
		self.color = _unpack_color(doc.get('color', 'FFFFFFFF'))


class Skin:
	def __init__ (self, rig, name, doc):
		self.rig = rig
		self.name = name

		self.slot_to_attachments = dict()
		for slot_name, attachments_dict in doc.iteritems():
			slot = rig.name_to_slot[slot_name]
			self.slot_to_attachments[slot] = dict(
				(key, Attachment(self, key,val))
				for key,val in attachments_dict.iteritems()
			)


class Attachment:
	def __init__ (self, skin, name, doc):
		self.skin = skin
		self.name = name

		# just handle ordinary images for now
		type = doc.get('type', 'region')
		assert type == 'region'

		self.image_id = doc.get('name', name)

		self.x = doc.get('x', 0)
		self.y = doc.get('y', 0)
		self.rotation = doc.get('rotation', 0)
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

		self.boneanimations = [ 
			BoneAnimation(self,k,v) 
			for k,v in doc.get('bones',{}).iteritems() 
		]
		self.bone_to_animation = dict((anim.bone, anim) for anim in self.boneanimations)

		def unpack_event(e):
			time = e['time']
			event = rig.name_to_event[ e['name'] ]
			intValue = e.get('int', event.intValue)
			floatValue = e.get('float', event.floatValue)
			stringValue = e.get('string', event.stringValue)
			return (time, event, intValue, floatValue, stringValue)
		self.eventanimation = map(unpack_event, doc.get('events', []))

		self.slotanimations = [
			SlotAnimation(self,k,v)
			for k,v in doc.get('slots',{}).iteritems()
		]
		self.slot_toanimation = dict((anim.slot, anim) for anim in self.slotanimations)

		if 'draworder' in doc:
			print 'WARNING: IGNORING DRAWORDER ANIMATIONS (for now)'

		duration = 0
		for anim in self.boneanimations: duration = max(duration, anim.duration)
		for anim in self.slotanimations: duration = max(duration, anim.duration)
		self.duration = duration

class BoneAnimation:
	def __init__(self, anim, bone_name, doc):
		self.anim = anim
		self.bone = anim.rig.name_to_bone[bone_name]

		# TODO: CURVES
		
		self.rotate_keys = [ 
			(k['time'], k['angle']) 
			for k in doc.get('rotate', []) 
		]

		self.translate_keys = [ 
			(k['time'], k['x'], k['y']) 
			for k in doc.get('translate', []) 
		]

		self.scale_keys = [ 
			(k['time'], k['x'], k['y']) 
			for k in doc.get('scale', []) 
		]

		duration = 0
		for time,_ in self.rotate_keys:	duration = max(duration, time)
		for time,_,_ in self.translate_keys: duration = max(duration, time)
		for time,_,_ in self.scale_keys: duration = max(duration, time)
		self.duration = duration

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


