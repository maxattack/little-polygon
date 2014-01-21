#include <littlepolygon_graphics.h>
#include <littlepolygon_pools.h>

struct Knot {
	const AffineMatrix *tform;
	vec2 attitude;
};

class SplineTree {
public:
	SplineTree();

	Knot* addKnot(const AffineMatrix *tform, vec2 attitude);
	void removeKnot(Knot *knot);

	void addSegment(const Knot *k0, const Knot *k1);
	void removeSegment(const Knot *k0, const Knot *k1);
	void draw(SplinePlotterRef splines, Color c, float strokeScale=1);
	void drawKnots(LinePlotterRef lines, Color c);

private:
	struct Segment {
		const Knot *k0;
		const Knot *k1;

		bool matches(const Knot *a0, const Knot *a1) const {
			return (k0 == a0 && k1 == a1) || (k0 == a1 && k1 == a0);
		}
	};

	BitsetPool<Knot, 256> knotPool;
	CompactPool<Segment, 1024> segmentPool;
};

