#ifndef QUADTREE_API_DLL
#define QUADTREE_API_DLL __declspec(dllimport)
#endif // QUANDTREE_API_DLL
#pragma once
#include "FreeList.h"
#include <list>

namespace LQT { // loose quad tree

	struct QUADTREE_API_DLL QTRect {
		float l, r, t, b;
		QTRect(float l = 0, float t = 0, float r = 0, float b = 0)
			: l(l), r(r), t(t), b(b) {}
		bool operator==(const QTRect& rhs) const {
			return rhs.l == l && rhs.r == r &&
				rhs.t == t && rhs.b == b;
		}
	};
	struct QUADTREE_API_DLL QTPoint {
		float x, y;
		QTPoint(float x = 0, float y = 0)
			: x(x), y(y) {}
		bool operator==(const QTPoint& rhs) const {
			return rhs.x == x && rhs.y == y;
		}
		QTPoint& operator/(const float& rhs) {
			x /= rhs; y /= rhs; return *this;
		}
		QTPoint& operator*(const float& rhs) {
			x *= rhs; y *= rhs; return *this;
		}
		QTPoint& operator+(const QTPoint& rhs) {
			x += rhs.x; y += rhs.y; return *this;
		}
		QTPoint& operator-(const QTPoint& rhs) {
			x -= rhs.x; y -= rhs.y; return *this;
		}
	};
	struct QUADTREE_API_DLL QNode {
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
	struct QUADTREE_API_DLL QNodeEle {
		QTRect rect;
		void* vPtr;
		QNodeEle(const QTRect& rect = { 0, 0, 0, 0 }, void* vPtr = nullptr)
			: rect(rect), vPtr(vPtr) {}
		bool operator==(const QNodeEle& e) const {
			return e.rect == rect && e.vPtr == vPtr;
		}
	};
	struct QUADTREE_API_DLL QNodeElePtr {
		int eleIdx;
		// point to next eleptr index,
		// -1 means this is the end of the list
		int next;
		QNodeElePtr(int eleidx = -1, int next = -1)
			:eleIdx(eleidx), next(next) {}
	};

	inline QTPoint GetRectCenter(const QTRect& r) {
		return { (r.r - r.l) / 2 + r.l, (r.b - r.t) / 2 + r.t };
	}

	inline bool IsRectsIntersect(const QTRect& lhs, const QTRect& rhs) {
		return !(rhs.l > lhs.r || rhs.r < lhs.l || rhs.t > lhs.b || rhs.b < lhs.t);
	}

	inline QTRect& UnionRect(QTRect& inout, const QTRect& in) {
		inout.l = in.l < inout.l ? in.l : inout.l;
		inout.r = in.r > inout.r ? in.r : inout.r;
		inout.t = in.t < inout.t ? in.t : inout.t;
		inout.b = in.b > inout.b ? in.b : inout.b;
		return inout;
	}

	inline bool IsPointInsideRect(const QTRect& rect, const QTPoint& point) {
		return !(rect.l > point.x || rect.r < point.x || rect.t > point.y || rect.b < point.y);
	}

	template class QUADTREE_API_DLL FreeList<QNodeEle>;
	template class QUADTREE_API_DLL FreeList<QNodeElePtr>;

	class QUADTREE_API_DLL QuadTree {
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
		void insert2(const QTPoint& cp, int cnIdx,
			const QTRect& xcp, int xndIdx = 0, int depth = 1);
		void insert4Nodes(int&); // insert 4 new nodes;
		//int insert4Nodes();
		// erase childs of nodes's element which index is idx
		// and reset its node's property to default value
		void eraseNodes(const int& idx);
		// to find out the leaf which include cp
		void queryLeaf(const QTPoint& cp, int& nodeIdx);
		inline void updateAABBSinceInsert(const int& xndIdx, const int& cnIdx);
		QTRect cleanupHelper(int idx, int& child);
		void cleanupHelper();
	private:
		FreeList<QNodeElePtr> elePtrs;
		FreeList<QNodeEle> eles;
		std::vector<QNode> nodes;
		int free_node;
		int maxDepth;
		int maxElePerLeaf;
		QTRect rootRect;
	};

}
