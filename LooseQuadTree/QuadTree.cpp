#ifndef QUADTREE_API_DLL
#define QUADTREE_API_DLL __declspec(dllexport)
#endif
#include "QuadTree.h"

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
		if (nChild == 0) { // all children are empty
			free_node = node.first_child;
			node.first_child = -1;
			node.count = 0;
		}
	}
	return retRect;
}

// no-recursion of cleanup. It may need some optimization
void QuadTree::cleanupHelper() {
	struct Reg {
		const int idx;
		int pt; // when pt == 4, reg should pop;
		Reg(int idx = -1, int pt = 0) : idx(idx), pt(pt) {}
	};
	std::vector<Reg> toProcess;
	std::vector<Reg> tempReg;
	toProcess.push_back({ 0 });
	while (toProcess.size()) {
		Reg curReg = toProcess.back();
		toProcess.pop_back();
		// root node is updated cleanup is finished;
		if (curReg.idx == 0 && (curReg.pt == 4 || curReg.pt == -4)) break;
		QNode & node = nodes[curReg.idx];
		if (curReg.pt == 0) {
			if (node.count == -1) { // it's a branch
				tempReg.push_back(curReg);
				toProcess.push_back({ node.first_child + 0 });
				toProcess.push_back({ node.first_child + 1 });
				toProcess.push_back({ node.first_child + 2 });
				toProcess.push_back({ node.first_child + 3 });
			}
			else { // it's a leaf
				   // update leaf's data
				int child = node.first_child;
				node.count = 0;
				node.aabbRect.l = node.aabbRect.r = node.aabbRect.t = node.aabbRect.b = 0;
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
				// update parent's data
				if (!tempReg.size()) continue;
				Reg& parentReg = tempReg.back();
				QNode& parent = nodes[parentReg.idx];
				if (node.count) {
					if (parentReg.pt <= 0) {
						parentReg.pt = 1 - parentReg.pt;
						parent.aabbRect = node.aabbRect;
					}
					else {
						++parentReg.pt;
						UnionRect(parent.aabbRect, node.aabbRect);
					}
				}
				else {
					if (parentReg.pt <= 0) --parentReg.pt;
					else ++parentReg.pt;
				}
				if (parentReg.pt == -4) {
					parent.aabbRect.l = parent.aabbRect.r = parent.aabbRect.t = parent.aabbRect.b = 0;
					parent.count = 0;
					nodes[parent.first_child].first_child = free_node;
					free_node = parent.first_child;
					parent.first_child = -1;
					toProcess.push_back(parentReg);
					tempReg.pop_back();
				}
				else if (parentReg.pt == 4) {
					toProcess.push_back(parentReg);
					tempReg.pop_back();
				}
			}
		}
		else if (curReg.pt == 4) {
			Reg& parentReg = tempReg.back();
			QNode& parent = nodes[parentReg.idx];
			if (parentReg.pt <= 0) {
				parentReg.pt = 1 - parentReg.pt;
				parent.aabbRect = node.aabbRect;
			}
			else {
				++parentReg.pt;
				UnionRect(parent.aabbRect, node.aabbRect);
			}
			if (parentReg.pt == 4) {
				toProcess.push_back(parentReg);
				tempReg.pop_back();
			}
		}
		else if (curReg.pt == -4) {
			// update parent's data
			Reg& parentReg = tempReg.back();
			QNode& parent = nodes[parentReg.idx];
			if (parentReg.pt <= 0) --parentReg.pt;
			else ++parentReg.pt;
			if (parentReg.pt == -4) {
				parent.aabbRect.l = parent.aabbRect.r = parent.aabbRect.t = parent.aabbRect.b = 0;
				parent.count = 0;
				nodes[parent.first_child].first_child = free_node;
				free_node = parent.first_child;
				parent.first_child = -1;
				toProcess.push_back(parentReg);
				tempReg.pop_back();
			}
			else if (parentReg.pt == 4) {
				toProcess.push_back(parentReg);
				tempReg.pop_back();
			}
		}
		else { throw("ERROR"); }
	}
}