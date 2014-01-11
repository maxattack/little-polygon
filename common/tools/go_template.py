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
import copy

# A lot of my parsing here is kinda roughshod/postel-ish.  Apologies T__T;;

def isPositionString(pos):
	return isinstance(pos, str) \
	and pos.startswith('(') \
	and pos.endswith(')') \
	and ',' in pos

def isColor(val):
	return isinstance(val,dict) \
	and 'r' in val and 'g' in val and 'b' in val

def unpackPosition(pos):
	if not isPositionString(pos): return (0,0)
	x,y = pos[1:-1].split(',')[:2]
	try:
		return (int(x), int(y))
	except:
		return (0,0)

def unpackValue(val):
	if isPositionString(val):
		return unpackPosition(val)
	# elif isColor(val):
	# 	r, g, b = int(val['r']), int(val['g']), int(val['b'])
	# 	return (r << 16) + (g << 8) + (b)
	else:
		return val

class GoTemplate:
	def __init__(self, key, val):
		
		# process formatted key
		ktype, self.name = key.split('/')[:2]
		self.concrete = ktype.endswith('!')
		self.type = ktype[:-1] if self.concrete else ktype

		# process val (steamrolling type :P)
		self.hasEnabled = 'enabled' in val
		self.hasPosition = 'position' in val
		self.enabled = bool(val['enabled']) if self.hasEnabled else False
		self.position = unpackPosition(val['position']) if self.hasPosition else (0,0)

		# populate components
		self.components = dict((c.name,c) for c in (
			GoComponent(self, key, val)
			for key,val in val.iteritems()
			if '/' in key and isinstance(val,dict)
		))

		self.parent = None
		self.propertiesConcatenated = False

	def isRoot(self):
		return self.type == "go"

	def concatenateProperties(self):
		if self.propertiesConcatenated: 
			return
		if self.parent is not None:
			self.parent.concatenateProperties()
			if not self.hasEnabled: self.enabled = self.parent.enabled
			if not self.hasPosition: self.position = self.parent.position
			for name,component in self.parent.components.iteritems():
				if name in self.components:
					self.components[name].concatenate(component)
				else:
					self.components[name] = copy.deepcopy(component)
		self.propertiesConcatenated = True

class GoComponent:
	def __init__(self, template, key, val):
		self.template = template
		self.type, self.name = key.split('/')[:2]
		self.properties = dict((p,unpackValue(v)) for p,v in val.iteritems())

	def concatenate(self, component):
		for key, val in component.properties.iteritems():
			if not key in self.properties:
				self.properties[key] = val


def load(path):

	# load templates
	with open(path, 'r') as f: 
		doc = yaml.load(f.read())
		assert isinstance(doc, dict)
		templates = [ 
			GoTemplate(key, val) 
			for key,val in doc.iteritems() 
			if '/' in key and isinstance(val,dict)
		]

	# hash by name and verify they're all unique
	nameToTemplate = dict((t.name, t) for t in templates)	
	assert len(templates) == len(nameToTemplate)

	# identify leaf nodes and build a parent-reference tree back to roots
	leafTemplates = [t for t in templates if t.concrete]
	openTemplates = [t for t in leafTemplates if not t.isRoot()]
	while len(openTemplates) > 0:
		template = openTemplates.pop()
		template.parent = nameToTemplate[template.type]
		if not template.parent.isRoot():
			openTemplates.append(template.parent)

	# now starting at the leaves, we want to "pull down" properties from the root
	for template in leafTemplates:
		template.concatenateProperties()
		print "----------------------------------------"
		print template.name
		print "Pos:", template.position
		print "Enabled:", template.enabled
		for key,component in template.components.iteritems():
			print "- %s : %s" % (key, component.type)
			for pkey,pval in component.properties.iteritems():
				print "  %s: %s" % (pkey, str(pval))


	# dedup leaf components


	


if __name__ == '__main__':
	print "testing templates..."
	load('squids/assets/templates.yaml')


