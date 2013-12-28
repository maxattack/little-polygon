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

#pragma once
#include "littlepolygon.h"

// A garbage-bin of additional helpful utilities not necessarily required by LPAAT,
// which I've accumulated and rolled over from project to project :P

//--------------------------------------------------------------------------------
// COROUTINE MACROS
//--------------------------------------------------------------------------------

#define COROUTINE_PARAMETER           int _line;
#define COROUTINE_RESET               _line=0;
#define COROUTINE_DISABLE             _line=-1;
#define COROUTINE_ACTIVE              (_line!=-1)
#define COROUTINE_BEGIN               switch(_line){case 0:;
#define COROUTINE_YIELD               {_line=__LINE__; return; case __LINE__:;}
#define COROUTINE_YIELD_RESULT(_x)    {_line=__LINE__;return (_x); case __LINE__:;}
#define COROUTINE_END                 _line=-1;default:;}

//--------------------------------------------------------------------------------
// GEOMETRY UTILITIES
//--------------------------------------------------------------------------------

// handy constants
#ifndef M_PI
#define M_PI 3.14159265359
#endif
#define M_TAU             (M_PI+M_PI)
#define M_COLINEAR_SLOP   (0.0001f)

// easy to use two-dimensional vector with shorthand operators
struct vec2 {
	float x, y;
	
	vec2() {}
	vec2(float ax, float ay) : x(ax), y(ay) {}

	#ifdef BOX2D_H
	// helpers if we're using box2D
	vec2(b2Vec2 v) : x(v.x), y(v.y) {}
	operator b2Vec2() { return b2Vec2(x, y); }
	#endif

	float real() const         { return x; }
	float imag() const         { return y; }
	float norm() const         { return x*x + y*y; }
	float manhattan() const    { return fabs(x)+fabs(y); }
	float magnitude() const    { return sqrtf(norm()); }
	vec2 conjugate() const     { return vec2(x,-y); }
	float radians() const      { return atan2f(y,x); }
	vec2 reflection() const    { return vec2(y,x); }
	vec2 anticlockwise() const { return vec2(-y, x); }
	vec2 clockwise() const     { return vec2(y, -x); }
	vec2 normalized() const    { return (*this) / magnitude(); }

	vec2 operator+(vec2 q) const  { return vec2(x+q.x, y+q.y); }
	vec2 operator-(vec2 q) const  { return vec2(x-q.x, y-q.y); }
	vec2 operator*(vec2 q) const  { return vec2(x*q.x, y*q.y); }
	vec2 operator/(vec2 q) const  { return vec2(x/q.x, y/q.y); }

	vec2 operator-() const        { return vec2(-x, -y); }
	vec2 operator*(float k) const { return vec2(k*x, k*y); }
	vec2 operator/(float k) const { return vec2(x/k, y/k); }

	vec2 operator +=(vec2 u)  { x+=u.x; y+=u.y; return *this; }
	vec2 operator -=(vec2 u)  { x-=u.x; y-=u.y; return *this; }
	vec2 operator *=(vec2 u)  { x*=u.x; y*=u.y; return *this; }
	vec2 operator /=(vec2 u)  { x/=u.x; y/=u.y; return *this; }

	vec2 operator *=(float k) { x*=k; y*=k; return *this; }
	vec2 operator /=(float k) { x/=k; y/=k; return *this; }
};

// I hate all those 2s cluttering up math code :P	
inline vec2 vec(float ax, float ay) { return vec2(ax, ay); }

// vec2 helpers
inline vec2 operator*(float k, vec2 q) { return vec2(k*q.x, k*q.y); }
inline float dot(vec2 u, vec2 v) { return u.x*v.x + u.y*v.y; }
inline float cross(vec2 u, vec2 v) { return u.x * v.y - v.x* u.y; }
inline vec2 lerp(vec2 u, vec2 v, float t) { return u + t * (v - u); }
inline vec2 slerp(vec2 u, vec2 v, float t) {
	float theta = acosf(dot(u,v));
	float s = 1.f/sinf(theta);
	return (sinf((1-t)*theta)*s)*u + (sinf(t*theta)*s)*v;
}

// linear range methods
inline float clamp(float u, float lo=0.f, float hi=1.f) { return u<lo?lo:u>hi?hi:u; }
inline float lerp(float u, float v, float t) { return u + t * (v-u); }

// complex multiplication
inline vec2 cmul(vec2 u, vec2 v) { return vec2(u.x*v.x-u.y*v.y, u.x*v.y+u.y*v.x); }

// complex division
inline vec2 cdiv(vec2 u, vec2 v) {
	float normInv = 1.0f/v.norm();
	return vec2((u.x*v.x+u.y*v.y)*normInv, (v.x*u.y-u.x*v.y)*normInv);
}

// polar -> linear conversion
inline vec2 polar(float radius, float radians) { return radius * vec2(cosf(radians), sinf(radians)); }

// easing functions
inline float easeOut2(float u) {
	u=1.0-u;
	return 1.0 - u*u;
}
inline float easeOut4(float u) {
	u=1.0-u;
	return 1.0 - u*u*u*u;
}
inline float easeInOutBack(float t, float s=1.70158f) { return (s+1)*t*t*t - s*t*t; }
inline float easeTowards(float curr, float target, float easing, float dt) { return curr + powf(easing, clamp(60*dt)) * (target - curr); }
inline vec2 easeTowards(vec2 curr, vec2 target, float easing, float dt) { float k = powf(easing, clamp(60*dt)); return curr + k * (target - curr); }

// random number functions
inline int randInt(int x) { return rand() % x; }
inline double randomValue() { return rand() / double(RAND_MAX); }
inline double randomValue(double u, double v) { return u + randomValue() * (v - u); }
inline double expovariate(double avgDuration) { return -avgDuration * log(1.0 - randomValue(0.0000001, 0.999999)); }
	
// handling radians sanely
inline float normalizeAngle(float radians) {
	radians = fmodf(radians, M_TAU);
	return radians < 0 ? radians + M_TAU : radians;
}
inline float radianDiff(float lhs, float rhs) {
	float result = normalizeAngle(lhs - rhs);
	if (result > M_PI) {
		return result - M_TAU;
	} else if (result < -M_PI) {
		return result + M_TAU;
	} else {
		return result;
	}
}
inline float easeRadians(float curr, float target, float easing, float dt) {
	return curr + powf(easing, clamp(60*dt)) * radianDiff(target, curr);
}

// I USE THIS A LOT O____O
inline float parabola(float x) {
	x = 1 - x - x;
	return 1.0f - x * x;
}

// line-line intersection test
bool linearIntersection(vec2 u0, vec2 u1, vec2 v0, vec2 v1, float& u);
bool linearIntersection(vec2 u0, vec2 u1, vec2 v0, vec2 v1, float& u, float& v);

// some common curves
vec2 quadraticBezier(vec2 p0, vec2 p1, vec2 p2, float u);
vec2 quadraticBezierDeriv(vec2 p0, vec2 p1, vec2 p2, float u);
vec2 cubicBezier(vec2 p0, vec2 p1, vec2 p2, vec2 p3, float u);
vec2 cubicBezierDeriv(vec2 p0, vec2 p1, vec2 p2, vec2 p3, float u);
vec2 cubicHermite(vec2 p0, vec2 m0, vec2 p1, vec2 m1, float u);
vec2 cubicHermiteDeriv(vec2 p0, vec2 m0, vec2 p1, vec2 m1, float u);

// Axis-Aligned Bounding Box
struct AABB {
	float x, y, w, h;
	
	AABB() {}
	AABB(vec2 pos, vec2 sz) : x(pos.x), y(pos.y), w(sz.x), h(sz.y) {}
	AABB(float ax, float ay, float aw, float ah) : x(ax), y(ay), w(aw), h(ah) {}

	vec2 size() const { return vec(w,h); }
	vec2 extentMin() const { return vec(x,y); }
	vec2 extentMax() const { return extentMin() + size(); }
	
	bool contains(vec2 p) { return p.x > x && p.y > y && p.x < x+w && p.y < y+h; }
	AABB inflate(float pad) { return AABB( x-pad, y-pad, w+pad+pad, h+pad+pad ); }
};

inline AABB unionOf(AABB u, AABB v) { return AABB(MIN(u.x, v.x), MIN(u.y, v.y), MAX(u.x+u.w, v.x+v.w), MAX(u.y+u.h, v.y+v.h)); }

// integral pair (e.g. tile locations)
struct ivec2 {
	int x, y;
	
	ivec2() {}
	ivec2(int ax, int ay) : x(ax), y(ay) {}
	
	inline vec2 toFloat() const { return vec(x, y); }
	inline ivec2 operator+(const ivec2& rhs) const { return ivec2(x+rhs.x, y+rhs.y); }
	inline ivec2 operator-(const ivec2& rhs) const { return ivec2(x-rhs.x, y-rhs.y); }
	inline ivec2 operator*(const ivec2& rhs) const { return ivec2(x*rhs.x, y*rhs.y); }
	inline ivec2 operator/(const ivec2& rhs) const { return ivec2(x/rhs.x, y/rhs.y); }
	
	inline bool operator==(const ivec2& rhs) const { return x == rhs.x && y == rhs.y; }
	
	// assuming raster-coordinates (y points "down")
	ivec2 above() const { return ivec2(x,y-1); }
	ivec2 below() const { return ivec2(x,y+1); }
};

inline ivec2 ivec(int x, int y) { return ivec2(x,y); }
inline ivec2 floor(vec2 v) { return ivec( int(v.x), int(v.y) ); }
inline ivec2 round(vec2 v) { return floor(v + vec(0.5f, 0.5f)); }

//--------------------------------------------------------------------------------
// COLOR HELPER
//--------------------------------------------------------------------------------

struct Color {
	union {
		uint32_t abgr; // messed-up opengl byte order :P
		struct {
			uint8_t r, g, b, a; // little endian
		};
	};

	Color() {}
	Color(uint8_t ar, uint8_t ag, uint8_t ab, uint8_t aa=0xff) : r(ar), g(ag), b(ab), a(aa) {}

	#ifdef BOX2D_H
	// helpers if we're using box2D
	Color(b2Color c) : r(0xff*c.r), g(0xff*c.g), b(0xff*c.b), a(0xff) {}
	operator b2Color() { return b2Color(red(), green(), blue()); }
	#endif

	float red() { return r * (1.f/255.f); }
	float green() { return g * (1.f/255.f); }
	float blue() { return b * (1.f/255.f); }
	float alpha() { return a * (1.f/255.f); }
	void toHSV(float *h, float *s, float *v);
};

// handy verbose constructors
inline Color rgba(uint32_t hex) { return Color( (0xff000000&hex)>>24, (0xff0000&hex)>>16, (0xff00&hex)>>8, 0xff&hex ); }
inline Color rgb(uint32_t hex) { return Color( (0xff0000&hex)>>16, (0xff00&hex)>>8, 0xff&hex, 0xff ); }
inline Color rgb(float r, float g, float b) { return Color(0xff*r, 0xff*g, 0xff*b); }
inline Color rgba(float r, float g, float b, float a) { return Color(0xff*r, 0xff*g, 0xff*b, 0xff*a); }
inline Color rgba(Color c, float a) { c.a = 0xff * a; return c; }
Color hsv(float h, float s, float v);
inline Color hsva(float h, float s, float v, float a) { return rgba(hsv(h,s,v), a); }

// interpolate in linear-color space
inline Color lerp(Color a, Color b, float u) {
	return rgba(
		lerp(a.red(), b.red(), u),
		lerp(a.green(), b.green(), u),
		lerp(a.blue(), b.blue(), u),
		lerp(a.alpha(), b.alpha(), u)
	);
}

//--------------------------------------------------------------------------------
// ANIMATION UTILITIES
//--------------------------------------------------------------------------------

inline int pingPong(int i, int n) {
	i  = i % (n + n - 2);
	return i >= n ? 2 * (n-1) - i : i;
}	

//--------------------------------------------------------------------------------
// SDL2 CONTEXT SETUP AND RAII FINALIZATION
//--------------------------------------------------------------------------------

class Context {
private:
	static Context *pInst;
	SDL_Window *pWindow;

public:
	Context(const char *caption, int w=0, int h=0);
	~Context();

	static inline Context *inst() { return pInst; }
	
	inline SDL_Window *window() const { 
		return pWindow; 
	}	
	
	ivec2 windowSize() const {
		// In general I prefer to not wrap any SDL methods just to save on typing,
		// but this is such a common one that that I made an exception.
		ivec2 result;
		SDL_GetWindowSize(window(), &result.x, &result.y);
		return result;
	}
};

//--------------------------------------------------------------------------------
// GRAPHICS HELPERS
//--------------------------------------------------------------------------------

void setCanvas(GLuint uMVP, vec2 canvasSize, vec2 canvasOffset);

//--------------------------------------------------------------------------------
// SPRITE BATCH UTILITY
// This object can render lots of sprites in a small number of batched draw calls
// by coalescing adjacent draws into larger logical draws.
//--------------------------------------------------------------------------------

#define SPRITE_CAPACITY 64

class SpriteBatch {
public:
	// Construct a batch with an internal draw-queue for "capacity" man sprites
	SpriteBatch();
	~SpriteBatch();

	// Call this method to initialize the graphics context state.  Coordinates are
	// set to a orthogonal projection matrix, and some basic settings like blending are
	// enabled.  Any additional state changes can be set *after* this function but *before*
	// issuing any draw calls.
	void begin(vec2 canvasSize, vec2 canvasOffset=vec(0,0));

	// Draw the given image.  Will potentially cause a draw call to actually be emitted
	// to the graphics device if: (i) the buffer has reached capacity or (ii) the texture 
	// atlas has changed.  Color transforms *can* be batched, because they are encoded
	// in the vertices, not in shader uniforms.
	void drawImage(ImageAsset *image, vec2 position, int frame=0, Color color=rgba(0x00000000));

	// Draw the given image, with it's local coordinates transformed by the given
	// "attitude" vector using complex multiplication, which supports rotation and
	// uniform scale
	void drawImageTransformed(ImageAsset *image, vec2 position, vec2 attitude, int frame=0, Color color=rgba(0x00000000));

	// Draw the given image with the given rotation (internally calls drawImageTransformed)
	void drawImageRotated(ImageAsset *image, vec2 position, float radians, int frame=0, Color color=rgba(0x00000000));

	// Draw the given image with the given non-uniform local scale
	void drawImageScaled(ImageAsset *image, vec2 position, vec2 k, int frame=0, Color color=rgba(0x00000000));
	
	// Draw the given texture using the texture-atlas for a specific font.
	void drawLabel(FontAsset *font, vec2 p, Color c, const char *msg);

	// Draw centered texture by measuing each line and subtracting half the advance
	void drawLabelCentered(FontAsset *font, vec2 p, Color c, const char *msg);

	// Commit the current draw queue and return the graphics context state to it's
	// canonical form, to play nice with other renderers.
	void end();

private:
	struct Vertex {
		vec2 position;
		vec2 uv;
		Color color;
		
		inline void set(vec2 p, vec2 u, Color c) {
			position = p;
			uv = u;
			color = c;
		}

	};


	// draw queue status
	int count;

	GLuint prog;
	GLuint vert;
	GLuint frag;
	GLuint uMVP;
	GLuint uAtlas;
	GLuint aPosition;
	GLuint aUV;
	GLuint aColor;	

	// invariant: these two fields need to be adjacent
	GLuint elementBuf;
	GLuint arrayBuf;
	
	Vertex workingBuffer[4 * SPRITE_CAPACITY]; // four corners
	TextureAsset *workingTexture;

	// private helper methods
	void setTextureAtlas(TextureAsset* texture);
	void commitBatch();
	void plotGlyph(const GlyphAsset& g, float x, float y, float h, Color c);
};

//--------------------------------------------------------------------------------
// LINE PLOTTER
// This is mainly a debugging tool for things like b2DebugDraw and diagnostics,
// so it's not exactly ninja'd for performance.
//--------------------------------------------------------------------------------

#define LINE_PLOTTER_CAPACITY 16

class LinePlotter {
public:
	LinePlotter();
	~LinePlotter();

	void begin(vec2 canvasSize, vec2 canvasOffset=vec(0,0));
	void plot(vec2 p0, vec2 p1, Color c);
	void end();

private:
	struct Vertex {
		vec2 position;
		Color color;
		inline void set(vec2 p, Color c) { position = p; color = c; }
	};


	Vertex vertices[ 2 * LINE_PLOTTER_CAPACITY ];
	int count;
	GLuint prog;
	GLuint vert;
	GLuint frag;
	GLuint uMVP;
	GLuint aPosition;
	GLuint aColor;

	void commitBatch();	
};

//--------------------------------------------------------------------------------
// TIMER UTILS
//--------------------------------------------------------------------------------

struct Timer {
	// sdl ticks since the timer started
	int ticks;
	int deltaTicks;

	// virtually scaled "seconds" since the timer started
	double timeScale;
	double scaledTime;
	double scaledDeltaTime;

	void init() {
		ticks = SDL_GetTicks();
		deltaTicks = 0;
		timeScale = 1;
		scaledTime = 0;
	}

	void tick() {
		deltaTicks = SDL_GetTicks() - ticks;
		scaledDeltaTime = timeScale * deltaSeconds();
		ticks += deltaTicks;
		scaledTime += scaledDeltaTime;
	}

	double seconds() const { return 0.001 * ticks; }
	double deltaSeconds() const { return 0.001 * deltaTicks; }
};

//--------------------------------------------------------------------------------
// SIMPLE TEMPLATE QUEUE
//--------------------------------------------------------------------------------

template<typename T, int N>
class Queue {
private:
	int n;
	int i;
	T buf[N];

public:
	Queue() : n(0), i(0) {}
	
	int capacity() const { return N; }
	int count() const { return n; }
	bool empty() const { return n == 0; }
	bool full() const { return n == N; }
	
	void enqueue(const T& val) {
		ASSERT(n < N);
		buf[(i + n) % N] = val;
		n++;
	}
	
	T dequeue() {
		ASSERT(n > 0);
		T result = buf[i];
		n--;
		i = (i+1) % N;
		return result;
	}

	T peekNext() {
		ASSERT(n > 0);
		return buf[i];
	}
	
	T peekLast() {
		ASSERT(n > 0);
		return buf[(i+n-1) % N];
	}
	
	bool tryDequeue(T* outValue) {
		if (n > 0) {
			*outValue = dequeue();
			return true;
		} else {
			return false;
		}
	}
	
	class Iterator {
	private:
		Queue *q;
		int idx;
		
	public:
		Iterator(Queue& queue) : q(&queue), idx(-1) {
		}
		
		bool step(T* outValue) {
			idx++;
			if (idx >= q->n) { return false; }
			*outValue = q->buf[(q->i + idx) % N];
			return true;
		}
	};
};

//--------------------------------------------------------------------------------
// SIMPLE TEMPLATE LIST/STACK
//--------------------------------------------------------------------------------

template<typename T, int N>
class List {
private:
	int n;
	T buf[N];

public:
	List() : n(0) {}
	
	int capacity() const { return N; }
	int count() const { return n; }
	size_t rawCapacity() const { return N * sizeof(T); }
	size_t rawSize() const { return n * sizeof(T); }
	bool empty() const { return n == 0; }
	bool full() const { return n == N; }

	T* begin() const { return buf; }
	T* end() const { return buf+n; }
	
	T get(int i) const {
		ASSERT(i >= 0);
		ASSERT(i < n);
		return buf[i];
	}

	T& operator[](int i) {
		ASSERT(i >= 0);
		ASSERT(i < n);
		return buf[i];
	}
	
	void clear() {
		n = 0;
	}
	
	void append(const T& val) {
		ASSERT(n < N);
		buf[n] = val;
		n++;
	}
	
	T& peekFirst() {
		ASSERT(n > 0);
		return buf[0];
	}
	
	T& peekLast() {
		ASSERT(n > 0);
		return buf[n-1];
	}
	
	T& push() {
		ASSERT(n < N);
		n++;
		return buf[n-1];
	}
	
	T pop() {
		ASSERT(n > 0);
		n--;
		return buf[n];
	}
	
	void removeAt(int i) {
		ASSERT(i >= 0);
		ASSERT(i < n);
		for(int j=i+1; j<n; ++j) {
			buf[j-1] = buf[j];
		}
		n--;
	}
	
	void insertAt(const T& val, int i) {
		ASSERT(i >= 0);
		ASSERT(i <= n);
		ASSERT(n < N);
		if (i == n) {
			push(val);
		} else {
			for(int j=n; j > i; --j) {
				buf[j] = buf[j-1];
			}
			buf[i] = val;
			n++;
		}
	}
	
	int indexOf(const T* val) const {
		ASSERT(val >= buf);
		ASSERT(val < buf + n);
		return int(val - buf);
	}
	
	int find(const T& val) const {
		for(int i=0; i<n; ++i) {
			if (buf[i] == val) { return i; }
		}
		return -1;
	}
	
	bool contains(const T& val) const {
		return find(val) != -1;
	}
};

//--------------------------------------------------------------------------------
// SIMPLE OBJECT POOL TEMPLATES
// Both pools store all the active objects in a compact array for fast 
// batch-processing.  When items are removed it's slot is exchanged with 
// the last item.
//--------------------------------------------------------------------------------

#define MAX_POOL_CAPACITY (64*1024)

typedef uint32_t ID;

struct PoolSlot {
    ID id;
    uint16_t index;
    uint16_t next;
};

struct Poolable {
	ID id;
};

// This pool is endowed with a storage-fixed ID table 
// for looking up objects "by reference."  Objects must
// have a public "id" field (easily accomplished by subclassing
// Poolable above).
template<typename T, int N>
class Pool {
private:
    uint32_t mCount;
    T mRecords[N];
    PoolSlot mSlots[N];
    uint16_t mFreelistEnqueue;
    uint16_t mFreelistDequeue;
	
public:
    Pool() : mCount(0), mFreelistEnqueue(N-1), mFreelistDequeue(0) {
        STATIC_ASSERT(N < MAX_POOL_CAPACITY);
        // initialize the free queue linked-list
        for(unsigned i=0; i<N; ++i) {
            mSlots[i].id = i;
            mSlots[i].next = i+1;
        }
    }
	
	bool isEmpty() const {
		return mCount == 0;
	}
	
	bool isFull() const {
		return mCount == N;
	}
	
    bool isActive(ID id) const {
        // use the lower-bits to find the record
        return mSlots[id & 0xffff].id == id;
    }
	
    T& operator[](ID id) {
        ASSERT(isActive(id)); 
        return mRecords[mSlots[id & 0xffff].index]; 
    }

    ID takeOut() {
        ASSERT(mCount < N);
        // dequeue a new index record - we do this in FIFO order so that
        // we don't "thrash" a record with interleaved add-remove calls
        // and use up the higher-order bits of the id
        PoolSlot &slot = mSlots[mFreelistDequeue];
		mFreelistDequeue = slot.next;
        // push a new record into the buffer
        slot.index = mCount++;
        // write the id to the record
        mRecords[slot.index].id = slot.id;
		return slot.id;
    }

    void putBack(ID id) {
        // assuming IDs are valid in production
        ASSERT(isActive(id));
        // lookup the index record
        PoolSlot &slot = mSlots[id & 0xffff];
		// move the last record into this slot
		T& record = mRecords[slot.index];
		record = mRecords[--mCount];
		// update the index from the moved record
		mSlots[record.id & 0xffff].index = slot.index;
        // increment the higher-order bits of the id (a fingerprint)
        slot.id += 0x10000;
		if (mCount == N-1) {
			mFreelistEnqueue = id & 0xffff;
			mFreelistDequeue = id & 0xffff;
		} else {
			mSlots[mFreelistEnqueue].next = id & 0xffff;
			mFreelistEnqueue = id & 0xffff;
		}
    }
    
	int count() const { return mCount; }

    uint16_t indexOf(ID id) { 
        return mSlots[id & 0xffff].index; 
    }

    T* begin() { 
        return mRecords; 
    }

    T* end() { 
        return mRecords + mCount; 
    }
};

// The anonymous pool is lighter weight than the regular pool because
// objects are not identified uniquely, but treated as a group.  Pointers
// returns from takeOut() are only valid until another non-const method on the pool
// is called.
template<typename T, int N>
class AnonymousPool {
private:
    uint32_t mCount;
    T mRecords[N];
	
public:
	AnonymousPool() : mCount(0) {
	}
	
	int count() const { return mCount; }
	bool isFull() const { return mCount == N; }
	T* begin() { return mRecords; }
	T* end() { return mRecords+mCount; }
	
	T* takeOut() {
		ASSERT(mCount < N);
		mCount++;
		return mRecords + (mCount-1);
	}
	
	void putBack(T* t) {
		int n = t - mRecords;
		ASSERT(n < mCount);
		mCount--;
		mRecords[n] = mRecords[mCount];
	}
	
	void drain() {
		mCount = 0;
	}
};


