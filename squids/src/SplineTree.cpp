#include "SplineTree.h"
#include <algorithm>

SplineTree::SplineTree() {
}

Knot* SplineTree::addKnot(const AffineMatrix *tform, vec2 attitude) {
	auto result = knotPool.alloc();
	result->tform = tform;
	result->attitude = attitude;
	return result;
}

void SplineTree::removeKnot(Knot *knot) {
	// remove any segments that start or end with this
	auto p = segmentPool.begin();
	while(p != segmentPool.end()) {
		if (p->k0 == knot || p->k1 == knot) {
			segmentPool.release(p);
		} else {
			++p;
		}
	}
	knotPool.release(knot);
}

void SplineTree::addSegment(const Knot *k0, const Knot *k1) {
	auto p = std::find_if(
		segmentPool.begin(), 
		segmentPool.end(), 
		[k0,k1](const Segment &s) { return s.matches(k0,k1); }
	);
	if (p == segmentPool.end()) {
		auto s = segmentPool.alloc();
		s->k0 = k0;
		s->k1 = k1;
	}
}

void SplineTree::removeSegment(const Knot *k0, const Knot *k1) {
	auto p = std::find_if(
		segmentPool.begin(), 
		segmentPool.end(), 
		[k0,k1](const Segment &s) { return s.matches(k0,k1); }
	);
	if (p != segmentPool.end()) {
		segmentPool.release(p);
	}
}

void SplineTree::draw(SplinePlotterRef splines, Color c, float ss) {
	for (auto& s : segmentPool) {
		auto p0 = s.k0->tform->t;
		auto p1 = s.k1->tform->t;
		auto u0 = s.k0->tform->transformVector(s.k0->attitude);
		auto u1 = s.k1->tform->transformVector(s.k1->attitude);
		splines.plot(
			hermiteMatrix(
				vec4f(p0.x, p0.y, 0, 0),
				vec4f(p1.x, p1.y, 0, 0),
				vec4f(u0.x, u0.y, 0, 0),
				vec4f(u1.x, u1.y, 0, 0)
			),
			eccentricStroke(ss * 12, ss * -6, ss * 12),
			c
		);
	}
}

void SplineTree::drawKnots(LinePlotterRef lines, Color c) {
	Knot *p;
	for(auto i=knotPool.listSlots(); i.next(p);) {
		auto p0 = p->tform->t;
		auto u = 0.3333 * p->tform->transformVector(p->attitude);		
		// plot a little square
		lines.plot(p0+vec(-4,-4), p0+vec(4,-4), c);
		lines.plot(p0+vec(4,-4), p0+vec(4,4), c);
		lines.plot(p0+vec(4,4), p0+vec(-4,4), c);
		lines.plot(p0+vec(-4,4), p0+vec(-4,-4), c);

		// plot tangent
		lines.plot(p0, p0+u, c);
		
		// plot a little arrow
		lines.plot(p0+u, p0 + u + 8*(u.clockwise()-u).normalized(), c);
		lines.plot(p0+u, p0 + u + 8*(u.anticlockwise()-u).normalized(), c);
	}
}