#pragma once
#include "FreeList.h"
#include <list>
struct QTRect {
	float l, r, t, b;
	QTRect(float l = 0, float t = 0, float r = 0, float b = 0)
		: l(l), r(r), t(t), b(b) {}
	bool operator==(const QTRect& rhs) const {
		return rhs.l == l && rhs.r == r &&
			rhs.t == t && rhs.b == b;
	}
};

inline QTRect& UnionRect(QTRect& inout, const QTRect& in) {
	inout.l = in.l < inout.l ? in.l : inout.l;
	inout.r = in.r > inout.r ? in.r : inout.r;
	inout.t = in.t < inout.t ? in.t : inout.t;
	inout.b = in.b > inout.b ? in.b : inout.b;
	return inout;
}

struct QTPoint {
	float x, y;
	QTPoint(float x = 0, float y = 0)
		: x(x), y(y) {}
	bool operator==(const QTPoint& rhs) const {
		return rhs.x == x && rhs.y == y;
	}
	QTPoint& operator/(const float& rhs) {
		x /= rhs; y /= rhs;return *this;}
	QTPoint& operator*(const float& rhs) {
		x *= rhs; y *= rhs;return *this;}
	QTPoint& operator+(const QTPoint& rhs) {
		x += rhs.x; y += rhs.y; return *this;}
	QTPoint& operator-(const QTPoint& rhs) {
		x -= rhs.x; y -= rhs.y; return *this;}
};

inline QTPoint GetRectCenter(const QTRect& r) {
	return { (r.r - r.l) / 2 + r.l, (r.b - r.t) / 2 + r.t };
}

inline bool IsRectsIntersect(const QTRect& lhs, const QTRect& rhs) {
	return !(rhs.l > lhs.r || rhs.r < lhs.l || rhs.t > lhs.b || rhs.b < lhs.t);
}

inline bool IsPointInsideRect(const QTRect& rect, const QTPoint& point) {
	return !(rect.l > point.x || rect.r < point.x || rect.t > point.y || rect.b < point.y);
}
 
struct QNode {
	// it will be the index of the first sub branch
	// or it will be the first ele index
	int first_child;
	// loose AABB
	QTRect aabbRect;
	// count = -1 if this node is a branch or
	// it's a leaf and count means the number of ele
	int count;
	QNode(int first_child = -1, int count = 0)
		: first_child(first_child), count(count) {}
};
struct QNodeEle {
	QTRect rect;
	void* vPtr;
	QNodeEle(QTRect rect = { 0, 0, 0, 0 }, void* vPtr = nullptr)
		: rect(rect), vPtr(vPtr) {}
	bool operator==(const QNodeEle& e) const {
		return e.rect == rect && e.vPtr == vPtr;
	}
};
struct QNodeElePtr {
	int eleIdx;
	// point to next eleptr index,
	// -1 means this is the end of the list
	int next;
	QNodeElePtr(int eleidx = -1, int next = -1)
		:eleIdx(eleidx), next(next) {}
};

class QuadTree {
public:
	QuadTree(QTRect rect, int maxDepth = 3,
		int maxElePerLeaf = 4);
	void Insert(const QNodeEle& ele);
	bool Erase(const QNodeEle& ele);
	bool Query(const QTRect& rect, std::list<QNodeEle>& retList);
	bool Query(const QTPoint& point, std::list<QNodeEle>& retList);
	void Cleanup(); // Cleanup empty branch and update branches aabbRect
private:
	void insert(const QTPoint& cp, int cnIdx,
		const QTPoint& xcp, int xndIdx,
		int depth);
	// to find out the leaf which include cp
	void queryLeaf(const QTPoint& cp, int& nodeIdx);
	inline void updateAABBSinceInsert(int xndIdx, int& cnIdx);
	QTRect cleanupHelper(int idx, int& child);
private:
	FreeList<QNodeElePtr> elePtrs;
	FreeList<QNodeEle> eles;
	std::vector<QNode> nodes;
	int free_node;
	int maxDepth;
	int maxElePerLeaf;
	QTRect rootRect;
};

QuadTree::QuadTree(QTRect rect, int maxDepth,	int maxElePerLeaf)
	: rootRect(rect), free_node(-1), maxDepth(maxDepth), maxElePerLeaf(maxElePerLeaf) {
	QNode root;
	nodes.push_back(root);
}

void QuadTree::Insert(const QNodeEle& ele) {
	QNodeElePtr qnep;
	qnep.eleIdx = eles.Insert(ele);
	insert(GetRectCenter(rootRect),
		0,
		GetRectCenter(ele.rect),
		elePtrs.Insert(qnep), 1);
}

bool QuadTree::Erase(const QNodeEle& ele) {
	int leafNodeIdx = 0;
	queryLeaf(GetRectCenter(ele.rect), leafNodeIdx);
	if (nodes[leafNodeIdx].count == 0) return false;
	int* cur = &nodes[leafNodeIdx].first_child;
	while (*cur != -1) {
		QNodeEle& xele = eles[elePtrs[*cur].eleIdx];
		if (xele == ele) {
			// erase element
			eles.Erase(elePtrs[*cur].eleIdx);
			int temp = *cur;
			// set last node's next pointer to current node's next
			*cur = elePtrs[*cur].next;
			// erase pointer
			elePtrs.Erase(temp);
			--nodes[leafNodeIdx].count;
			return true;
		}
		cur = &elePtrs[*cur].next;
	}
	return false;
}

bool QuadTree::Query(const QTRect& rect, std::list<QNodeEle>& retList) {
	std::vector<int> toProcess = std::vector<int>(96);
	toProcess.push_back(0);
	while (toProcess.size()) {
		QNode& node = nodes[toProcess.back()];
		toProcess.pop_back();
		if (!IsRectsIntersect(rect, node.aabbRect)) continue;
		if (node.count != -1) {
			// it's leaf
			int child = node.first_child;
			while (child != -1) {
				if (IsRectsIntersect(eles[elePtrs[child].eleIdx].rect, rect)) retList.push_back(eles[elePtrs[child].eleIdx]);
				child = elePtrs[child].next;
			}
		}
		else { // it's branch
			toProcess.push_back(node.first_child + 0);
			toProcess.push_back(node.first_child + 1);
			toProcess.push_back(node.first_child + 2);
			toProcess.push_back(node.first_child + 3);
		}
	}
	return retList.size();
}

bool QuadTree::Query(const QTPoint& point, std::list<QNodeEle>& retList) {
	std::vector<int> toProcess = std::vector<int>(96);
	toProcess.push_back(0);
	while (toProcess.size())
	{
		QNode& node = nodes[toProcess.back()];
		toProcess.pop_back();
		if (!IsPointInsideRect(node.aabbRect, point)) continue;
		if (node.count != -1) {
			// it's leaf
			int child = node.first_child;
			while (child != -1) {
				if (IsPointInsideRect(eles[elePtrs[child].eleIdx].rect, point)) retList.push_back(eles[elePtrs[child].eleIdx]);
				child = elePtrs[child].next;
			}
		}
		else {
			// it's branch
			toProcess.push_back(node.first_child + 0);
			toProcess.push_back(node.first_child + 1);
			toProcess.push_back(node.first_child + 2);
			toProcess.push_back(node.first_child + 3);
		}
	}
	return retList.size();
}

void QuadTree::Cleanup() {
	int temp;
	cleanupHelper(0, temp);
}

/*=======
! PRIVATE !
=======*/
void QuadTree::insert(const QTPoint& cp, int cnIdx,
	const QTPoint& xcp, int xndIdx,
	int depth) {
	// cp: center of current node
	// cnode : current node, it may be a branch or a leaf
	// depth : current node's depth. Based depth is 1
	// xcp: center of the new element
	// xndIdx: new element ptr index
	QTPoint offset;
	offset.x = (rootRect.r - rootRect.l) / (2 * (depth + 1));
	offset.y = (rootRect.b - rootRect.t) / (2 * (depth + 1));
	if (depth == maxDepth) {
		// current node must be a leaf
		QNode& cnd = nodes[cnIdx];
		elePtrs[xndIdx].next = cnd.first_child;
		cnd.first_child = xndIdx;
		++cnd.count;
	}
	else if (nodes[cnIdx].count != -1) {
		// current node is a leaf
		if (nodes[cnIdx].count < maxElePerLeaf) {
			// insert new elePtr to it
			QNode& cnd = nodes[cnIdx];
			elePtrs[xndIdx].next = cnd.first_child;
			cnd.first_child = xndIdx;
			++cnd.count;
		}
		else {
			// current node need to split and become a branch
			QNode lt, ld, rt, rd;
			int index;
			if (free_node != -1) {
				index = free_node;
				free_node = nodes[free_node].first_child;
				nodes[index + 0] = lt;
				nodes[index + 1] = ld;
				nodes[index + 2] = rt;
				nodes[index + 3] = rd;
			}
			else {
				index = nodes.size();
				nodes.push_back(lt); // index + 0
				nodes.push_back(ld); // index + 1
				nodes.push_back(rt); // index + 2
				nodes.push_back(rd); // index + 3
			}
			while (nodes[cnIdx].count > 0) {
				QNode& cnd = nodes[cnIdx];
				int childPtr = cnd.first_child;
				cnd.first_child = elePtrs[cnd.first_child].next;
				QTRect childRect = eles[elePtrs[childPtr].eleIdx].rect;
				QTPoint point = { (childRect.l + childRect.r) / 2, (childRect.t + childRect.b) / 2 };
				if (point.x > cp.x) {
					// in the right side
					if (point.y > cp.y) insert({ cp.x + offset.x, cp.y + offset.y }, index + 3, point, childPtr, depth + 1);
					else insert({ cp.x + offset.x, cp.y - offset.y }, index + 2, point, childPtr, depth + 1);
				}
				else {
					// in the left side
					if (point.y > cp.y) insert({ cp.x - offset.x, cp.y + offset.y }, index + 1, point, childPtr, depth + 1);
					else insert({ cp.x - offset.x, cp.y - offset.y }, index + 0, point, childPtr, depth + 1);
				}
				--cnd.count;
			}
			if (xcp.x > cp.x) {
				if (xcp.y > cp.y) insert({ cp.x + offset.x, cp.y + offset.y }, index + 3, xcp, xndIdx, depth + 1);
				else insert({ cp.x + offset.x, cp.y - offset.y }, index + 2, xcp, xndIdx, depth + 1);
			}
			else {
				if (xcp.y > cp.y) insert({ cp.x - offset.x, cp.y + offset.y }, index + 1, xcp, xndIdx, depth + 1);
				else insert({ cp.x - offset.x, cp.y - offset.y }, index + 0, xcp, xndIdx, depth + 1);
			}
			nodes[cnIdx].count = -1;
			nodes[cnIdx].first_child = index;
		}
	}
	else {
		// current node is a branch
		// so it need to insert to its child
		QNode& cnd = nodes[cnIdx];
		if (xcp.x > cp.x) {
			if (xcp.y > cp.y) insert({ cp.x + offset.x, cp.y + offset.y }, cnd.first_child + 3, xcp, xndIdx, depth + 1);
			else insert({ cp.x + offset.x, cp.y - offset.y }, cnd.first_child + 2, xcp, xndIdx, depth + 1);
		}
		else {
			if (xcp.y > cp.y) insert({ cp.x - offset.x, cp.y + offset.y }, cnd.first_child + 1, xcp, xndIdx, depth + 1);
			else insert({ cp.x - offset.x, cp.y - offset.y }, cnd.first_child + 0, xcp, xndIdx, depth + 1);
		}
	}
	updateAABBSinceInsert(xndIdx, cnIdx);
}

void QuadTree::queryLeaf(const QTPoint& cp, int& nodeIdx) {
	QTPoint offset = { (rootRect.r - rootRect.l) / 2, (rootRect.b - rootRect.t) / 2 };
	QTPoint xcp = GetRectCenter(rootRect);
	nodeIdx = 0;
	while (nodes[nodeIdx].count == -1) {
		QNode& nd = nodes[nodeIdx];
		offset = offset / 2;
		if (cp.x > xcp.x) {
			// right side
			if (cp.y > xcp.y) { // down
				nodeIdx = nd.first_child + 3; xcp = xcp + offset;
			}
			else { // up
				nodeIdx = nd.first_child + 2; xcp = xcp + QTPoint(offset.x, -offset.y);
			}
		}
		else {
			// left side
			if (cp.y > xcp.y) { // down
				nodeIdx = nd.first_child + 1; xcp = xcp + QTPoint(-offset.x, offset.y);
			}
			else { // up
				nodeIdx = nd.first_child + 0; xcp = xcp - offset;
			}
		}
	}
}

inline void QuadTree::updateAABBSinceInsert(int xndIdx, int& cnIdx) {
	// xndldx: the index of the elePtr
	QTRect& rect = eles[elePtrs[xndIdx].eleIdx].rect;
	QNode& cnd = nodes[cnIdx];
	if (cnd.count == 1) {
		cnd.aabbRect = rect;
	}
	else {
		UnionRect(cnd.aabbRect, rect);
	}
}

QTRect QuadTree::cleanupHelper(int idx, int& nChild) {
	QTRect retRect;
	QNode& node = nodes[idx];
	nChild = 0;
	if (node.count != -1) { // it's leaf
		int child = node.first_child;
		if (child != -1) {
			++nChild;
			retRect = eles[elePtrs[child].eleIdx].rect;
		} else return retRect;
		child = elePtrs[child].next;
		while (child != -1) {
			++nChild;
			QTRect& rect = eles[elePtrs[child].eleIdx].rect;
			UnionRect(retRect, rect);
			child = elePtrs[child].next;
		}
	}
	else { // it's branch
		int c1, c2, c3, c4;
		retRect = cleanupHelper(nodes[idx].first_child + 0, c1); nChild += c1;
		UnionRect(retRect, cleanupHelper(nodes[idx].first_child + 1, c2)); nChild += c2;
		UnionRect(retRect, cleanupHelper(nodes[idx].first_child + 2, c3)); nChild += c3;
		UnionRect(retRect, cleanupHelper(nodes[idx].first_child + 3, c4)); nChild += c4;
		node.aabbRect = retRect;
		if (nChild == 0) { // all children are empty
			free_node = node.first_child;
			node.first_child = -1;
			node.count = 0;
		}
	}
	return retRect;
}