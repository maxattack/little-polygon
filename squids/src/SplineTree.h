#include <littlepolygon_fk.h>
#include <littlepolygon_graphics.h>

#define SEGMENT_CAPACITY 1024

class SplineTree {
public:
	SplineTree(FkContext *displayTree);

	void addSegment(FkNode *start, FkNode *end);
	void removeSegment(FkNode *start, FkNode *end);
	void draw(SplinePlotter *splines, Color c);

private:
	int mCount;

	struct Segment {
		FkNode *start;
		FkNode *end;
	};

	Segment mSegments[SEGMENT_CAPACITY];

	Segment* find(FkNode *start, FkNode *end);
};

