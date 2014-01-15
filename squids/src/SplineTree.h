#include <littlepolygon_fk.h>
#include <littlepolygon_graphics.h>

#define SEGMENT_CAPACITY 1024

class SplineTree {
public:
	SplineTree(FkContext *displayTree);

	void addSegment(const AffineMatrix *t0, const AffineMatrix *t1);
	void removeSegment(const AffineMatrix *t0, const AffineMatrix *t1);
	void draw(SplinePlotter *splines, Color c, float strokeScale=1);

private:
	int mCount;

	struct Segment {
		const AffineMatrix *t0;
		const AffineMatrix *t1;
	};

	Segment mSegments[SEGMENT_CAPACITY];

	Segment* find(const AffineMatrix *t0, const AffineMatrix *t1);
};

