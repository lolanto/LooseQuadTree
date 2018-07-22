#include "QuadTree.hpp"
#include <vector>
#include <list>
#include <random>
#include <iostream>

QTRect CreateRandomRect(const QTRect& range) {
	static std::random_device rdev;
	QTRect retRect;
	retRect.l = rdev() % (int)(range.r - range.l) + range.l;
	retRect.r = rdev() % (int)(range.r - retRect.l) + retRect.l;
	retRect.t = rdev() % (int)(range.b - range.t) + range.t;
	retRect.b = rdev() % (int)(range.b - retRect.t) + retRect.t;
	return retRect;
}

void main() {
	const QTRect qtRect = { 10, 12, 120, 200 };
	const unsigned int numOfEle = 1000;
	const float proOfErase = 0.4f;
	const float proOfCleanup = 0.1f;
	std::random_device rdev;

	QuadTree qt = QuadTree(qtRect);
	std::vector<QNodeEle> randEle;
	std::vector<bool> reg;
	for (unsigned int i = 0; i < numOfEle; ++i)
		randEle.push_back({ CreateRandomRect(qtRect) });
	for (unsigned int i = 0; i < numOfEle; ++i) {
		float p = (float)(rdev() % 100) / 100.0f;
		if (p < proOfCleanup) {
			qt.Cleanup();
		}
		else if (p < proOfErase && i) {
			int t = rdev() % i;
			qt.Erase(randEle[t]);
			reg[t] = false;
		}
		qt.Insert(randEle[i]);
		reg.push_back(true);
	}
	qt.Cleanup();
	//std::cout << "check~" << std::endl;
	//for (unsigned int i = 0; i < numOfEle; ++i) {
	//	QNodeEle& node = randEle[i];
	//	QTPoint p = GetRectCenter(node.rect);
	//	std::list<QNodeEle> list;
	//	qt.Query(p, list);
	//	bool cp = false;
	//	for (auto& iter : list) {
	//		if (iter == node) {
	//			cp = true;
	//			break;
	//		}
	//	}
	//	if (cp != reg[i]) std::cout << "wrond!" << std::endl;
	//}
	//system("pause");
}
