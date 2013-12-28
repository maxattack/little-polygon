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
from pyglsl import ShaderOptimizer

LINE_BOTH, LINE_VERT, LINE_FRAG = xrange(3)

def prettify_name(name):
	return name[0].upper() + name[1:]

def extract_type_name(line):
	tokens = line.split(' ')
	return (tokens[-2], tokens[-1])

MAP_TYPE_SIG = {
	'vec2': 'float x, float y',
	'vec3': 'float x, float y, float z',
	'vec4': 'float x, float y, float z, float w',
	'float': 'float x',
	'mat2': 'const float* buf, bool transpose = 0',
	'mat3': 'const float* buf, bool transpose = 0',
	'mat4': 'const float* buf, bool transpose = 0',
	'mat2x3': 'const float* buf, bool transpose = 0',
	'mat3x2': 'const float* buf, bool transpose = 0',
	'mat2x4': 'const float* buf, bool transpose = 0',
	'mat4x2': 'const float* buf, bool transpose = 0',
	'mat3x4': 'const float* buf, bool transpose = 0',
	'mat4x3': 'const float* buf, bool transpose = 0',
}

MAP_TYPE_SUFFIX = {
	'vec2': '2f',
	'vec3': '3f',
	'vec4': '4f',
	'float': '1f',
	'mat2': 'Matrix2fv',
	'mat3': 'Matrix3fv',
	'mat4': 'Matrix4fv',
	'mat2x3': 'Matrix2x3fv',
	'mat3x2': 'Matrix3x2fv',
	'mat2x4': 'Matrix2x4fv',
	'mat4x2': 'Matrix4x2fv',
	'mat4x3': 'Matrix4x3fv',
	'mat3x4': 'Matrix3x4fv',
}

MAP_TYPE_ARGS = {
	'vec2': 'x, y',
	'vec3': 'x, y, z',
	'vec4': 'x, y, z, w',
	'float': 'x',
	'mat2': '1, transpose, buf',
	'mat3': '1, transpose, buf',
	'mat4': '1, transpose, buf',
	'mat2x3': '1, transpose, buf',
	'mat3x2': '1, transpose, buf',
	'mat2x4': '1, transpose, buf',
	'mat4x2': '1, transpose, buf',
	'mat4x3': '1, transpose, buf',
	'mat3x4': '1, transpose, buf',
}

MAP_TYPE_COMP = {
	'vec2': 2,
	'vec3': 3,
	'vec4': 4,
	'float': 1,
}

def literal(src):
	lines = ( '"%s\\n"' % line for line in src.split('\n') if line)
	return '\n\t'.join(lines)


def load_source(path):
	vlines = []
	flines = []
	status = LINE_BOTH
	with open(path, 'r') as file:
		for line in file:
			if line.startswith('#if VERTEX'):
				status = LINE_VERT
			elif line.startswith('#if FRAGMENT'):
				status = LINE_FRAG
			elif line.startswith('#else'):
				status = LINE_FRAG if status == LINE_VERT else LINE_VERT
			elif line.startswith('#endif'):
				status = LINE_BOTH
			elif line:
				if status != LINE_FRAG: vlines.append(line)
				if status != LINE_VERT: flines.append(line)
	return ''.join(vlines), ''.join(flines)

def compile(path, es):
	# load optimized shader
	shader_name = os.path.splitext(os.path.split(path)[1])[0]
	v,f = load_source(path)
	optimizer = ShaderOptimizer(es)
	v = optimizer.optimize_vert_shader(v)
	f = optimizer.optimize_frag_shader(f)
	if not v or not f:
		raise Exception("GLSL Syntax Error:" + path)
	return v, f	

def compileAndGenerateCode(path, es):
	v, f = compile(path, es)

	deduped_lines = set(v.split('\n') + f.split('\n'))
	attrib_lines = (l[:-1] for l in deduped_lines if l.startswith('attribute'))
	uniform_lines = (l[:-1] for l in deduped_lines if l.startswith('uniform'))

	attribs = [ extract_type_name(line) for line in attrib_lines ]
	uniforms = [ extract_type_name(line) for line in uniform_lines ]

	simple_uniforms = [ (t,n) for t,n in uniforms if not 'sampler' in t ]
	texture_names = [ (n) for t,n in uniforms if t == 'sampler2D' ]

	class_name = prettify_name(shader_name)+'Shader'
	print 'Writing', class_name

	# write header	
	hdr = []

	def sln(msg):
		hdr.append(msg)
		hdr.append('\n')
	
	def stab(msg):
		hdr.append('\t')
		hdr.append(msg)
		hdr.append('\n')

	sln('class %s : public Graphics::Shader {' % class_name)
	sln('private:')
	for _,name in attribs + uniforms:
		stab('GLuint m%s;' % prettify_name(name))
	stab('')
	sln('public:')
	stab('void initializeShader();\n')
	stab('')
	
	# direct handle getters
	for _,name in attribs + uniforms:
		stab('GLuint %sHandle() const { return m%s; };' % (name, prettify_name(name)))			
	stab('')

	# uniform setters
	for type,name in simple_uniforms:
		pretty = prettify_name(name)
		stab('void set%s(%s) {\n\t\tglUniform%s(m%s, %s);\n\t}\n' % (
			pretty, 
			MAP_TYPE_SIG[type], 
			MAP_TYPE_SUFFIX[type], 
			pretty, 
			MAP_TYPE_ARGS[type]
		))


	for name in texture_names:
		pretty = prettify_name(name)
		stab('void set%s(int activeTextureId) {\n\t\tglUniform1i(m%s, activeTextureId);\n\t}\n' % (pretty, pretty))


	# stab('void enableVertexArrays() {')
	# for type,name in attribs:
	# 	stab('\tglEnableVertexAttribArray(m%s);' % prettify_name(name))		
	# stab('}')

	# stab('void disableVertexArrays() {')
	# for type,name in attribs:
	# 	stab('\tglDisableVertexAttribArray(m%s);' % prettify_name(name))		
	# stab('}')

	for type,name in attribs:
		pretty = prettify_name(name)
		# hack for immediate-mode drawing...
		# stab('void plot%sVertex(%s) {\n\t\tglVertexAttrib%s(m%s, %s);\n\t}\n' % (
		# 	pretty,
		# 	MAP_TYPE_SIG[type],
		# 	MAP_TYPE_SUFFIX[type],
		# 	pretty,
		# 	MAP_TYPE_ARGS[type]
		# ))
		# stab('void enable%sArray() {\n\t\tglEnableVertexAttribArray(m%s);\n\t}\n' % (pretty, pretty))
		stab('void set%sPointer(float *p, int stride=0) {\n\t\tglVertexAttribPointer(m%s, %d, GL_FLOAT, GL_FALSE, stride, p);\n\t}\n' % (
			pretty,
			pretty,
			MAP_TYPE_COMP[type]
		))
	sln('};\n')

	hdr = ''.join(hdr)
	
	# write source
	src = []

	def sln(msg):
		src.append(msg)
		src.append('\n')
	
	def stab(msg):
		src.append('\t')
		src.append(msg)
		src.append('\n')

	sln('static const char s%sVertSrc[] = \n\t%s;\n' % (class_name, literal(v)))
	sln('static const char s%sFragSrc[] = \n\t%s;\n' % (class_name, literal(f)))
	
	# write ctor
	sln('void %s::initializeShader() {' % class_name)
	stab('compile(s%sVertSrc, s%sFragSrc);' % (class_name, class_name))
	for _,name in attribs:
		stab('m%s = glGetAttribLocation(progHandle(), "%s");' % (prettify_name(name), name))
	for _,name in uniforms:
		stab('m%s = glGetUniformLocation(progHandle(), "%s");' % (prettify_name(name), name))

	# sneaky attrib iterator
	stab('mAttribBegin = &m%s;' % prettify_name(attribs[0][1]))
	stab('mAttribEnd = &m%s+1;' % prettify_name(attribs[-1][1]))


	sln('}\n')

	src = ''.join(src)

	return class_name, hdr, src

if __name__ == '__main__':
	shaders = []
	for arg in sys.argv:
		if arg.endswith('.glsl'):
			shaders.append(arg)
	if not shaders:
		shaders = glob('*.glsl')
	for shader in shaders:
		v, f = compile(shader, '-es' in sys.argv)
		print "------"
		print "VERTEX (%s)" % shader
		print "------"
		print v
		print "--------"
		print "FRAGMENT (%s)" % shader
		print "--------"
		print f

		# class_name, hdr, src = compile(shader, '-es' in sys.argv)
		# with open(class_name+'.h', 'w') as h: 
		# 	h.write('#pragma once\n')
		# 	h.write('#include "Graphics.h"\n')
		# 	h.write('\n')
		# 	h.write('//' + '-'*80 + '\n')
		# 	h.write('// Generated by tools/compile_shaders.py\n')
		# 	h.write('//' + '-'*80 + '\n')
		# 	h.write('\n')
		# 	h.write(hdr)
		# with open(class_name+'.cpp', 'w') as s: 
		# 	s.write('#include "%s.h"\n' % class_name)
		# 	s.write('\n')
		# 	s.write('//' + '-'*80 + '\n')
		# 	s.write('// Generated by tools/compile_shaders.py\n')
		# 	s.write('//' + '-'*80 + '\n')
		# 	s.write('\n')
		# 	s.write(src)
