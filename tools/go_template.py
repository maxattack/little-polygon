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

#
# GAME OBJECT TEMPLATE COMPILER
#
# Inspects a YAML file and builds up a template-specialization heirarchy
# (templates specialize other templates, overriding and adding properties
# like in CSS).  The "concrete" templates are actually instantiated in the
# scene and have some special markup identifying them.  The result of the
# "load" method is a 2-tuple containing:
# (i) A list of GoTemplate instances whose properties have been fully
#     concatenated with their parents and the components deduplicated
#     with each other.
# (ii) A table of the deduplicated components which actually need to be
#      converted into assets for initializing components at runtime.

# A lot of my parsing here is kinda roughshod/postel-ish.  Apologies T__T;;

class GoTemplate:
	def __init__(self, key, val):
		
		# process formatted key
		ktype, self.name = key.split('/')[:2]
		self.concrete = ktype.endswith('!')
		self.type = ktype[:-1] if self.concrete else ktype

		# process val (steamrolling type :P)
		self.hasEnabled = 'enabled' in val
		self.hasPosition = 'position' in val
		self.enabled = val['enabled'] if self.hasEnabled else False
		self.position = val['position'] if self.hasPosition else [0,0]
		assert isinstance(self.enabled, bool)
		assert isinstance(self.position, list)

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
		self.properties = val
		self.dedupIndex = -1

	def concatenate(self, component):
		for key, val in component.properties.iteritems():
			if not key in self.properties:
				self.properties[key] = val

	def equiv(self, component):
		if self.type != component.type or len(self.properties) != len(component.properties): 
			return False
		sharedItems = set(self.properties.items()) & set(component.properties.items())
		return len(sharedItems) == len(self.properties)


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
		if '-v' in sys.argv:
			print "----------------------------------------"
			print template.name
			print "Pos:", template.position
			print "Enabled:", template.enabled
			for key,component in template.components.iteritems():
				print "- %s : %s" % (key, component.type)
				for pkey,pval in component.properties.iteritems():
					print "  %s: %s" % (pkey, str(pval))


	# dedup leaf components (store in a table hashed by their component type)
	dedupedComponents = {}
	for template in leafTemplates:
		for component in template.components.itervalues():
			# determine the dedup index
			if component.type in dedupedComponents:
				# check if we match any previously-registered components
				dedupList = dedupedComponents[component.type]
				for i,otherComponent in enumerate(dedupList):
					if component.equiv(otherComponent):
						component.dedupIndex = i
						break
				if component.dedupIndex == -1:
					# append this component to the list
					component.dedupIndex = len(dedupList)
					dedupList.append(component)
			else:
				# create a new list
				dedupedComponents[component.type] = [ component ]
				component.dedupIndex = 0

	return (leafTemplates, dedupedComponents)


	


if __name__ == '__main__':
	print "testing templates..."
	load('squids/assets/templates.yaml')


