#include "src/QuadTree.h"
#include <vector>
#include <list>
#include <random>
#include <iostream>
#include <Windows.h>

QTRect CreateRandomRect(const QTRect& range) {
	static std::random_device rdev;
	QTRect retRect;
	retRect.l = rdev() % (int)(range.r - range.l) + range.l;
	retRect.r = rdev() % (int)(range.r - retRect.l) + retRect.l;
	retRect.t = rdev() % (int)(range.b - range.t) + range.t;
	retRect.b = rdev() % (int)(range.b - retRect.t) + retRect.t;
	//if (retRect.l == retRect.r) retRect.r += 0.1f;
	//if (retRect.t == retRect.b) retRect.b += 0.1f;
	return retRect;
}

const QTRect qtRect = { 10, 10, 100, 100 };
const unsigned int numOfEle = 512;
const float proOfErase = 0.4f;
const float proOfCleanup = 0.1f;

std::vector<QNodeEle> randEle = std::vector<QNodeEle>(numOfEle);
std::vector<int> eraseReg = std::vector<int>(numOfEle);
std::vector<bool> eleExistReg = std::vector<bool>(numOfEle);
std::vector<bool> cleanupReg = std::vector<bool>(numOfEle);
std::random_device rdev;
void CreateTestData() {
	
	for (unsigned int i = 0; i < numOfEle; ++i) {
		randEle[i] = QNodeEle(CreateRandomRect(qtRect), (void*)i);
		float p = (float)(rdev() % 100) / 100.0f;
		if (p < proOfCleanup) {
			cleanupReg[i] = true;
			eraseReg[i] = -1;
		}
		else if (p < proOfErase && i) {
			int t = rdev() % i;
			eraseReg[i] = t;
			eleExistReg[t] = false;
		}
		else {
			cleanupReg[i] = false;
			eraseReg[i] = -1;
		}
		eleExistReg[i] = true;
	}
}

void StartTest() {
	QuadTree qt = QuadTree(qtRect);
	for (unsigned int i = 0; i < numOfEle; ++i) {
		if (eraseReg[i] != -1) qt.Erase(randEle[eraseReg[i]]);
		if (cleanupReg[i]) qt.Cleanup();
		qt.Insert(randEle[i]);
	}
	qt.Cleanup();
	std::cout << "check~" << std::endl;
	for (unsigned int i = 0; i < numOfEle; ++i) {
		QNodeEle& node = randEle[i];
		QTPoint p = GetRectCenter(node.rect);
		std::list<QNodeEle> list;
		qt.Query(p, list);
		bool cp = false;
		for (auto& iter : list) {
			if (iter == node) {
				cp = true;
				break;
			}
		}
		if (cp != eleExistReg[i]) {
			std::cout << "wrong!" << std::endl;
			throw("error");
		};
	}
}

void main() {
	LARGE_INTEGER BegainTime;
	LARGE_INTEGER EndTime;
	LARGE_INTEGER Frequency;
	QueryPerformanceFrequency(&Frequency);

	double reqTime = 0.0f;

	for (int i = 0; i < 1000; ++i) {
		CreateTestData();
		std::cout << "test: " << i << std::endl;
		QueryPerformanceCounter(&BegainTime);
		StartTest();
		QueryPerformanceCounter(&EndTime);
		reqTime += (double)(EndTime.QuadPart - BegainTime.QuadPart) / Frequency.QuadPart;
		std::cout << "运行时间（单位：s）：" << (double)(EndTime.QuadPart - BegainTime.QuadPart) / Frequency.QuadPart << std::endl;
	}

	std::cout << "req ave: " << reqTime / 1000 << std::endl;

	//QuadTree qt({ 0, 0, 15, 15 });
	//qt.Insert({ {0, 0, 0.5, 0.5} });
	//qt.Insert({ { 0.5, 0.5, 1.5, 1.5 } });
	//qt.Insert({ { 1.5, 1.5, 2, 2 } });
	//qt.Insert({ { 2, 2, 2.5, 2.5 } });
	//qt.Insert({ { 2.5, 2.5, 3, 3 } });

	system("pause");
}
