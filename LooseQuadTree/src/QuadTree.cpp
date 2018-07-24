#ifndef QUADTREE_API_DLL
#define QUADTREE_API_DLL __declspec(dllexport)
#endif
#include "QuadTree.h"
#include <deque>

namespace LQT {

	QuadTree::QuadTree(QTRect rect, int maxDepth, int maxElePerLeaf)
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
		//insert2(GetRectCenter(ele.rect), elePtrs.Insert(qnep), rootRect);
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
		std::deque<int> toProcess;
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
		std::vector<int> toProcess;
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
		//cleanupHelper();
	}

	/*=======
	! PRIVATE !
	=======*/

	inline void QuadTree::insert4Nodes(int& i) {
		if (free_node != -1) {
			i = free_node;
			free_node = nodes[i].first_child;
			nodes[i + 0] = QNode();
			nodes[i + 1] = QNode();
			nodes[i + 2] = QNode();
			nodes[i + 3] = QNode();
		}
		else {
			i = nodes.size();
			nodes.push_back({});
			nodes.push_back({});
			nodes.push_back({});
			nodes.push_back({});
		}
	}
	inline void QuadTree::eraseNodes(const int& idx) {
		nodes[idx].first_child = free_node;
		free_node = idx;
		nodes[idx].aabbRect = {};
		nodes[idx].count = 0;
		nodes[idx].first_child = -1;
	}

	void QuadTree::insert(const QTPoint& cp, int cnIdx,
		const QTPoint& xcp, int xndIdx,
		int depth) {
		// cp: center of current node
		// cnIdx : current node index, it may be a branch or a leaf
		// depth : current node's depth. Based depth is 1
		// xcp: center of the new element
		// xndIdx: new element ptr index
		QTPoint offset;
		offset.x = (rootRect.r - rootRect.l) / (2 * (depth + 1));
		offset.y = (rootRect.b - rootRect.t) / (2 * (depth + 1));
		QNode& cnd = nodes[cnIdx];
		if (depth == maxDepth) { // current node must be a leaf
			elePtrs[xndIdx].next = cnd.first_child;
			cnd.first_child = xndIdx;
			++cnd.count;
		}
		else if (cnd.count != -1) { // current node is a leaf
			if (cnd.count < maxElePerLeaf) { // insert new elePtr to it
				elePtrs[xndIdx].next = cnd.first_child;
				cnd.first_child = xndIdx;
				++cnd.count;
			}
			else { // current node need to split and become a branch
				cnd.count = -1;
				elePtrs[xndIdx].next = cnd.first_child;
				insert4Nodes(cnd.first_child);
				// after push some elements, variable node is invalid, since nodes's memory is changed
				int next;
				while (xndIdx != -1) {
					next = elePtrs[xndIdx].next;
					insert(cp, cnIdx, GetRectCenter(eles[elePtrs[xndIdx].eleIdx].rect), xndIdx, depth);
					xndIdx = next;
				}
				return;
			}
		}
		else { // current node is a branch, so it need to insert to its child
			if (xcp.x > cp.x) { // right side
				if (xcp.y > cp.y) insert({ cp.x + offset.x, cp.y + offset.y }, cnd.first_child + 3, xcp, xndIdx, depth + 1);
				else insert({ cp.x + offset.x, cp.y - offset.y }, cnd.first_child + 2, xcp, xndIdx, depth + 1);
			}
			else { // left side
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

	inline void QuadTree::updateAABBSinceInsert(const int& xndIdx, const int& cnIdx) {
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

	// recursion of cleanup
	QTRect QuadTree::cleanupHelper(int idx, int& nChild) {
		QTRect retRect;
		QNode& node = nodes[idx];
		nChild = 0;
		if (node.count != -1) { // it's leaf
			int child = node.first_child;
			if (child != -1) {
				++nChild;
				retRect = eles[elePtrs[child].eleIdx].rect;
			}
			else return retRect;
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
			if (nChild == 0) // all children are empty
				eraseNodes(idx);
		}
		return retRect;
	}

	// no-recursion of cleanup. It may need some optimization
	void QuadTree::cleanupHelper() {
		std::deque<int> toProcess;
		std::deque<QTRect*> regStack;
		toProcess.push_back(0);
		while (toProcess.size()) {
			QNode& node = nodes[toProcess.back()];
			if (node.count == -1) { // It's a branch without processed
				node.count = -2;
				toProcess.push_back(node.first_child + 0);
				toProcess.push_back(node.first_child + 1);
				toProcess.push_back(node.first_child + 2);
				toProcess.push_back(node.first_child + 3);
			}
			else if (node.count == -2) { // It's a branch with processed
				//toProcess.erase(toProcess.end() - 1);
				toProcess.pop_back();
				bool init = false;
				for (node.count = 0; node.count < 4; ++node.count) {
					const QTRect* reg = regStack.back();
					if (reg) {
						if (!init) { node.aabbRect = *reg; init = true; }
						else { UnionRect(node.aabbRect, *reg); }
					}
					//regStack.erase(regStack.end() - 1);
					regStack.pop_back();
				}
				if (init) { // not empty
					node.count = -1;
					regStack.push_back(&node.aabbRect);
				}
				else { // it's empty
					node.aabbRect.l = node.aabbRect.r = node.aabbRect.t = node.aabbRect.b = 0;
					nodes[node.first_child].first_child = free_node;
					free_node = node.first_child;
					node.first_child = -1;
					node.count = 0;
					regStack.push_back(nullptr);
				}
			}
			else { // It's a leaf
				//toProcess.erase(toProcess.end() - 1);
				toProcess.pop_back();
				int child = node.first_child;
				node.count = 0;
				if (child != -1) {
					++node.count;
					node.aabbRect = eles[elePtrs[child].eleIdx].rect;
					child = elePtrs[child].next;
					while (child != -1) {
						++node.count;
						UnionRect(node.aabbRect, eles[elePtrs[child].eleIdx].rect);
						child = elePtrs[child].next;
					}
				}
				else node.aabbRect.l = node.aabbRect.r = node.aabbRect.t = node.aabbRect.b = 0;
				if (node.count) regStack.push_back(&node.aabbRect);
				else regStack.push_back(nullptr);
			}
		}
	}
	// no-recursion of insert. It may need some optimization
	void QuadTree::insert2(const QTPoint& cp, int cnIdx,
		const QTRect& xrect, int xndIdx, int depth) {
		struct Reg {
			const QTRect rect; // node's rect NOT aabbrect
			int depth; // node's depth
			int idx; // node's index in nodes
			Reg(const QTRect& rect, int idx, int depth) : rect(rect), idx(idx), depth(depth) {}
		};
		struct PReg {
			const QTPoint point; // element's center
			int idx; // element's pointer's address in elePtrs
			PReg(const QTPoint& point, int idx) : point(point), idx(idx) {}
		};
		std::deque<PReg> toProcess;
		std::deque<Reg> toInsert;
		toProcess.push_back({ cp, cnIdx });
		toInsert.push_back({ xrect, xndIdx, depth });
		while (toProcess.size() && toInsert.size()) {
			const PReg& preg = toProcess.back();
			const Reg& reg = toInsert.back();
			QTPoint offset = { (reg.rect.r - reg.rect.l) / 2, (reg.rect.b - reg.rect.t) / 2 };
			QNode& node = nodes[reg.idx];
			if (IsPointInsideRect(reg.rect, preg.point)) {
				if (node.count == -1) { // it's a branch
					updateAABBSinceInsert(preg.idx, reg.idx);
					QTPoint&& center = GetRectCenter(reg.rect);
					if (preg.point.x > center.x) { // right side
						if (preg.point.y > center.y)// down side
							toInsert.push_back({ { center.x, center.y, center.x + offset.x, center.y + offset.y },
								node.first_child + 3, reg.depth + 1 });
						else // up side
							toInsert.push_back({ {center.x, center.y - offset.y, center.x + offset.x, center.y},
								node.first_child + 2, reg.depth + 1 });
					}
					else { // left side
						if (preg.point.y > center.y) // down side
							toInsert.push_back({ {center.x - offset.x, center.y, center.x, center.y + offset.y},
								node.first_child + 1, reg.depth + 1 });
						else {// up side
							toInsert.push_back({ {center.x - offset.x, center.y - offset.y, center.x, center.y},
								node.first_child + 0, reg.depth + 1 });
						}
					}
				}
				else if (node.count < maxElePerLeaf || reg.depth == maxDepth) { // it's a leaf can insert directly
					++node.count;
					updateAABBSinceInsert(preg.idx, reg.idx);
					elePtrs[preg.idx].next = node.first_child;
					node.first_child = preg.idx;
					toProcess.pop_back();
				}
				else { // it's a leaf but it's full of elements and need to split
					int child = node.first_child;
					while (child != -1) {
						toProcess.push_back({ GetRectCenter(eles[elePtrs[child].eleIdx].rect), child });
						child = elePtrs[child].next;
					}
					node.count = -1;
					insert4Nodes(node.first_child);
				}
			}
			else {
				toInsert.pop_back();
			}
		}
	}

}