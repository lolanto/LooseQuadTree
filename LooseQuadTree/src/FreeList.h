#pragma once
#include <vector>

namespace LQT { // loose quad tree

	template<typename T>
	class FreeList {
	public:
		FreeList() : free_node(-1) {}
		int Insert(const T& ele) {
			if (free_node != -1) {
				const int index = free_node;
				free_node = eles[index].next;
				eles[index].ele = ele;
				return index;
			}
			else {
				ListEle ne;
				ne.ele = ele;
				eles.push_back(ne);
				return eles.size() - 1;
			}
		}
		void Erase(const int& i) {
			if (i < 0) return;
			eles[i].next = free_node;
			free_node = i;
		}
		void Clear() {
			free_node = -1;
			eles.clear();
		}
		T& operator[](const unsigned int& i) {
			return eles[i].ele;
		}
		const T& operator[](const unsigned int& i) const {
			return eles[i].ele;
		}
	private:
		union ListEle
		{
			T ele;
			int next;
			ListEle() : next(-1) {}
		};
		int free_node;
		std::vector<ListEle> eles;
	};

}