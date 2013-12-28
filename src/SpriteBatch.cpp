// Little Polygon SDK
// Copyright (C) 2013 Max Kaufmann
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "littlepolygon_utils.h"

const GLchar* SPRITE_SHADER = R"GLSL(

varying mediump vec2 uv;
varying mediump vec4 color;

#if VERTEX

uniform mediump mat4 mvp;
attribute mediump vec2 aPosition;
attribute mediump vec2 aUv;
attribute mediump vec4 aColor;

void main() {
	gl_Position = mvp * vec4(aPosition, 0, 1);
	color = aColor;
	uv = aUv;
}

#else

uniform lowp sampler2D atlas;

void main() {
	vec4 baseColor = texture2D(atlas, uv);
	gl_FragColor = vec4(mix(baseColor.rgb, color.rgb, color.a), baseColor.a);
}

#endif

)GLSL";

SpriteBatch::SpriteBatch()  :
count(0),
workingTexture(0) {
	CHECK(ShaderAsset::compile(SPRITE_SHADER, &prog, &vert, &frag));
	glUseProgram(prog);
	uMVP = glGetUniformLocation(prog, "mvp");
	uAtlas = glGetUniformLocation(prog, "atlas");
	aPosition = glGetAttribLocation(prog, "aPosition");
	aUV = glGetAttribLocation(prog, "aUv");
	aColor = glGetAttribLocation(prog, "aColor");

	// setup element array buffer (should this be static?)
	uint16_t indices[6 * SPRITE_CAPACITY];
	for(int i=0; i<SPRITE_CAPACITY; ++i) {
		indices[6*i+0] = 4*i;
		indices[6*i+1] = 4*i+1;
		indices[6*i+2] = 4*i+2;
		indices[6*i+3] = 4*i+2;
		indices[6*i+4] = 4*i+1;
		indices[6*i+5] = 4*i+3;
	}
	glGenBuffers(2, &elementBuf); // also generates the arrayBuf
	                              // because it's adjacent
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuf);
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER, 
		6 * SPRITE_CAPACITY * sizeof(uint16_t), 
		indices, 
		GL_STATIC_DRAW
	);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

SpriteBatch::~SpriteBatch() {
	glDeleteBuffers(2, &elementBuf); 
	glDeleteProgram(prog);
	glDeleteShader(vert);
	glDeleteShader(frag);	
}

void SpriteBatch::begin(vec2 canvasSize, vec2 canvasOffset) {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	ASSERT(prog);
	glUseProgram(prog);
	glEnableVertexAttribArray(aPosition);
	glEnableVertexAttribArray(aUV);
	glEnableVertexAttribArray(aColor);

	setCanvas(uMVP, canvasSize, canvasOffset);

	// bind element buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuf);
	glBindBuffer(GL_ARRAY_BUFFER, arrayBuf);

	glVertexAttribPointer(aPosition, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
	glVertexAttribPointer(aUV, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)8);
	glVertexAttribPointer(aColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (GLvoid*)16);	
}

void SpriteBatch::drawImage(ImageAsset *img, vec2 pos, int frame, Color c) {
	setTextureAtlas(img->texture);
	Vertex *slice = &workingBuffer[4 * count];
	FrameAsset *fr = img->frame(frame);

	pos -= vec(fr->px, fr->py);
	
	slice[0].set(pos,                   vec(fr->u0, fr->v0), c);
	slice[1].set(pos+vec(0,fr->h),      vec(fr->u1, fr->v1), c);
	slice[2].set(pos+vec(fr->w, 0),     vec(fr->u2, fr->v2), c);
	slice[3].set(pos+vec(fr->w, fr->h), vec(fr->u3, fr->v3), c);

	++count;
}

void SpriteBatch::drawImageTransformed(ImageAsset *img, vec2 pos, vec2 u, int frame, Color c) {
	setTextureAtlas(img->texture);
	Vertex *slice = &workingBuffer[4 * count];
	FrameAsset *fr = img->frame(frame);

	vec2 p0 = -vec(fr->px, fr->py);
	vec2 p1 = p0 + vec(0, fr->h);
	vec2 p2 = p0 + vec(fr->w, 0);
	vec2 p3 = p0 + vec(fr->w, fr->h);

	slice[0].set(pos+cmul(p0,u), vec(fr->u0, fr->v0), c);
	slice[1].set(pos+cmul(p1,u), vec(fr->u1, fr->v1), c);
	slice[2].set(pos+cmul(p2,u), vec(fr->u2, fr->v2), c);
	slice[3].set(pos+cmul(p3,u), vec(fr->u3, fr->v3), c);

	++count;
}

void SpriteBatch::drawImageRotated(ImageAsset *img, vec2 pos, float radians, int f, Color c) {
	drawImageTransformed(img, pos, polar(1, radians), f, c);
}

void SpriteBatch::drawImageScaled(ImageAsset *img, vec2 pos, vec2 k, int frame, Color c) {
	setTextureAtlas(img->texture);
	Vertex *slice = &workingBuffer[4 * count];
	FrameAsset *fr = img->frame(frame);

	vec2 p0 = -vec(fr->px, fr->py);
	vec2 p1 = p0 + vec(0, fr->h);
	vec2 p2 = p0 + vec(fr->w, 0);
	vec2 p3 = p0 + vec(fr->w, fr->h);

	slice[0].set(pos+k*p0, vec(fr->u0, fr->v0), c);
	slice[1].set(pos+k*p1, vec(fr->u1, fr->v1), c);
	slice[2].set(pos+k*p2, vec(fr->u2, fr->v2), c);
	slice[3].set(pos+k*p3, vec(fr->u3, fr->v3), c);

	++count;	
}


#define UV_LABEL_SLOP (0.0001f)

void SpriteBatch::plotGlyph(const GlyphAsset& g, float x, float y, float h, Color c) {

	if (count == SPRITE_CAPACITY) {
		commitBatch();
	}

	Vertex *slice = &workingBuffer[4 * count];
	float k = 1.f / workingTexture->w;
	vec2 uv = k * vec(g.x, g.y);
	float du = k * (g.advance-UV_LABEL_SLOP);
	float dv = k * h;

	slice[0].set(vec(x,y), uv, c);	
	slice[1].set(vec(x,y+h), uv+vec(0,dv), c);	
	slice[2].set(vec(x+g.advance,y), uv+vec(du,0), c);	
	slice[3].set(vec(x+g.advance,y+h), uv+vec(du,dv), c);	

	++count;
}

void SpriteBatch::drawLabel(FontAsset *font, vec2 p, Color c, const char *msg) {
	setTextureAtlas( &(font->texture) );

	float px = p.x;
	float py = p.y;
	while(*msg) {
		if (*msg != '\n') {
			GlyphAsset g = font->getGlyph(*msg);
			plotGlyph(g, px, py, font->height, c);
			px += g.advance;
		} else {
			px = p.x;
			py += font->height;
		}
		++msg;
	}

}

void SpriteBatch::drawLabelCentered(FontAsset *font, vec2 p, Color c, const char *msg) {
	setTextureAtlas( &(font->texture) );
	float py = p.y;
	while(*msg) {
		int length;
		const char* next = font->measureLine(msg, &length);
		float px = p.x - (length>>1);
		while(msg != next) {
			GlyphAsset g = font->getGlyph(*msg);
			plotGlyph(g, px, py, font->height, c);
			px += g.advance;
			msg++;
		}
		if (*msg == '\n') {
			py += font->height;
			msg++;
		}
	}
}

void SpriteBatch::end() {
	if (count > 0) {
		commitBatch();
	}
	workingTexture = 0;
	glDisableVertexAttribArray(aPosition);
	glDisableVertexAttribArray(aUV);
	glDisableVertexAttribArray(aColor);
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void SpriteBatch::commitBatch() {
	ASSERT(count > 0);
	glBufferData(GL_ARRAY_BUFFER, 4 * count * sizeof(Vertex), workingBuffer, GL_DYNAMIC_DRAW);
	glDrawElements(GL_TRIANGLES, 6 * count, GL_UNSIGNED_SHORT, 0);
	count = 0;
}

void SpriteBatch::setTextureAtlas(TextureAsset *texture) {

	bool atlasChange = texture != workingTexture;
	
	// emit a draw call if we're at SPRITE_CAPACITY of if the atlas is changing.
	// (assuming that count will be 0 if workingTexture is null)
	if (count == SPRITE_CAPACITY || (count > 0 && atlasChange)) {
		commitBatch();
	}

	// check if we need to bind a new texture
	if (atlasChange) {
		texture->bind();
		workingTexture = texture;		
	}
}




