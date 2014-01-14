#include "SplineTree.h"

SplineTree::SplineTree(FkContext *context) : mDisplayTree(context), mCount(0) {
}

void SplineTree::addSegment(FkNode *start, FkNode *end) {
	ASSERT(mCount < SEGMENT_CAPACITY);
	if (!find(start, end)) {
		mSegments[mCount].start = start;
		mSegments[mCount].end = end;
		++mCount;
	}
}

void SplineTree::removeSegment(FkNode *start, FkNode *end) {
	auto s = find(start, end);
	if (s) {
		if (s != mSegments+(mCount-1)) { *s = mSegments[mCount-1]; }
		--mCount;
	}
}

void SplineTree::draw(SplinePlotter *splines, Color c) {
	for(auto s=mSegments; s<mSegments+mCount; ++s) {
		auto w0 = fkWorld(s->start);
		auto w1 = fkWorld(s->end);
		drawSpline(
			splines, 
			hermiteMatrix(
				vec4f(w0.t.x, w0.t.y, 0, 0),
				vec4f(w1.t.x, w1.t.y, 0, 0),
				vec4f(w0.u.x, w0.u.y, 0, 0),
				vec4f(w1.u.x, w1.u.y, 0, 0)
			),
			eccentricStroke(16, -12, 16),
			c
		);
	}
}

SplineTree::Segment* SplineTree::find(FkNode *start, FkNode *end) {
	for(auto s=mSegments; s<mSegments+mCount; ++s) {
		if ((s->start == start && s->end == end) ||
		    (s->start == end && s->end == start)) {
			return s;
		}
	}
	return 0;
}