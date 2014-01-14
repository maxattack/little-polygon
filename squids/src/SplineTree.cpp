#include "SplineTree.h"

SplineTree::SplineTree() : mCount(0) {
	mDisplayTree = createFkContext();
}

SplineTree::~SplineTree() {
	destroy(mDisplayTree);
}

void SplineTree::addSegment(Node *start, Node *end) {
	ASSERT(mCount < SEGMENT_CAPACITY);
	if (!find(start, end)) {
		mSegments[mCount].start = start;
		mSegments[mCount].end = end;
		++mCount;
	}
}

void SplineTree::removeSegment(Node *start, Node *end) {
	auto s = find(start, end);
	if (s) {
		if (s != mSegments+(mCount-1)) { *s = mSegments[mCount-1]; }
		--mCount;
	}
}

void SplineTree::draw(SplinePlotter *splines, Color c) {
	cacheWorldTransforms(mDisplayTree);
	for(auto s=mSegments; s<mSegments+mCount; ++s) {
		auto w0 = world(mDisplayTree, s->start);
		auto w1 = world(mDisplayTree, s->end);
		drawSpline(
			splines, 
			hermiteMatrix(
				vec4f(w0.t.x, w0.t.y, 0, 0),
				vec4f(w1.t.x, w1.t.y, 0, 0),
				vec4f(w0.u.x, w0.u.y, 0, 0),
				vec4f(w1.u.x, w1.u.y, 0, 0)
			),
			uniformStroke(16),//taperingStroke(w0.v.magnitude(), w1.v.magnitude()),
			c
		);
	}
}

SplineTree::Segment* SplineTree::find(Node *start, Node *end) {
	for(auto s=mSegments; s<mSegments+mCount; ++s) {
		if ((s->start == start && s->end == end) ||
		    (s->start == end && s->end == start)) {
			return s;
		}
	}
	return 0;
}