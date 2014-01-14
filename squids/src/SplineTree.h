#include <littlepolygon_fk.h>
#include <littlepolygon_graphics.h>

#define SEGMENT_CAPACITY 1024

class SplineTree {
public:
	SplineTree();
	~SplineTree();

	FkContext *displayTree() { return mDisplayTree; }
	void addSegment(Node *start, Node *end);
	void removeSegment(Node *start, Node *end);
	void draw(SplinePlotter *splines, Color c);

private:
	int mCount;
	FkContext *mDisplayTree;

	struct Segment {
		Node *start;
		Node *end;
	};

	Segment mSegments[SEGMENT_CAPACITY];

	Segment* find(Node *start, Node *end);
};

