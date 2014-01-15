#include "SplineTree.h"

SplineTree::SplineTree(FkContext *context) : mCount(0) {
}

void SplineTree::addSegment(const AffineMatrix *t0, const AffineMatrix *t1) {
	ASSERT(mCount < SEGMENT_CAPACITY);
	if (!find(t0, t1)) {
		mSegments[mCount].t0 = t0;
		mSegments[mCount].t1 = t1;
		++mCount;
	}
}

void SplineTree::removeSegment(const AffineMatrix *t0, const AffineMatrix *t1) {
	auto s = find(t0, t1);
	if (s) {
		if (s != mSegments+(mCount-1)) { *s = mSegments[mCount-1]; }
		--mCount;
	}
}

void SplineTree::draw(SplinePlotter *splines, Color c, float ss) {
	for(auto s=mSegments; s<mSegments+mCount; ++s) {
		drawSpline(
			splines, 
			hermiteMatrix(
				vec4f(s->t0->t.x, s->t0->t.y, 0, 0),
				vec4f(s->t1->t.x, s->t1->t.y, 0, 0),
				vec4f(s->t0->u.x, s->t0->u.y, 0, 0),
				vec4f(s->t1->u.x, s->t1->u.y, 0, 0)
			),
			eccentricStroke(ss * 12, ss * -6, ss * 12),
			c
		);
	}
}

SplineTree::Segment* SplineTree::find(const AffineMatrix *t0, const AffineMatrix *t1) {
	for(auto s=mSegments; s<mSegments+mCount; ++s) {
		if ((s->t0 == t0 && s->t1 == t1) ||
		    (s->t0 == t1 && s->t1 == t0)) {
			return s;
		}
	}
	return 0;
}