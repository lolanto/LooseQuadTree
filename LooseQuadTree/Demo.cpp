#include "src/QuadTree.h"
#include <map>
#include <iostream>
#include <string>
using namespace LQT;
using std::cout;
using std::endl;

class Listener {
public: virtual void Dispatch(void*) = 0;
};

class Element {
public:
	Element(const QTRect& rect) : m_rect(rect), m_isDirty(false), m_container(nullptr) {}
	const QTRect& GetRect() const { return m_rect; }
	void SetContainer(Listener* container) { m_container = container; }
	void Clear() { m_isDirty = false; }
	void SetPos(const QTPoint& offset) {
		if (!m_isDirty) {
			m_container->Dispatch(this);
			m_isDirty = true;
		}
		m_rect.l += offset.x; m_rect.r += offset.x;
		m_rect.t += offset.y; m_rect.b += offset.y;
	}
	void SetSize(const QTPoint& offset) {
		if (!m_isDirty) {
			m_container->Dispatch(this);
			m_isDirty = true;
		}
		m_rect.r += offset.x;
		m_rect.b += offset.y;
	}
protected:
	QTRect m_rect;
	bool m_isDirty;
	Listener* m_container;
};

class Container : public Listener{
public:
	Container(const QTRect& rect) : qt({ rect }) {}
	void Dispatch(void* e) {
		Element* ele = reinterpret_cast<Element*>(e);
		updates.insert(std::make_pair(ele, ele->GetRect()));
	}
	void Add(Element* e) {
		QNodeEle node(e->GetRect(), e);
		qt.Insert(node);
		e->SetContainer(this);
	}
	std::vector<Element*> Query(const QTPoint& point) {
		// update all elements
		for (auto& iter : updates) {
			qt.Erase({ iter.second, (void*)iter.first });
			qt.Insert({ iter.first->GetRect(), (void*)iter.first });
			iter.first->Clear();
		}
		updates.clear();
		// query quadtree
		std::list<QNodeEle> relist;
		qt.Query(point, relist);
		std::vector<Element*> reVec;
		for (auto& iter : relist)
			reVec.push_back(reinterpret_cast<Element*>(iter.vPtr));
		return reVec;
	}
private:
	QuadTree qt;
	std::map<Element*, const QTRect> updates;
};

class EleA : public Element {
	friend std::ostream& operator << (std::ostream& out, EleA& in);
public:
	EleA(const QTRect& rect, const char* name) : Element(rect), m_name(name) {}
private:
	std::string m_name;
};

std::ostream& operator<<(std::ostream& out, EleA& in) {
	out << "Element Name: " << in.m_name << endl;
	out << "Element Rect: (" << in.m_rect.l << ", " << in.m_rect.t << ") (" << in.m_rect.r << ", " << in.m_rect.b << ")" << endl;
	return out;
}

void main() {
	Container c({3, 3, 10, 10});
	EleA e({ 4, 4, 6, 5 }, "EleA");
	EleA b({ 6, 6, 8, 8 }, "EleB");
	c.Add(&e);
	c.Add(&b);
	
	cout << "Add element EleA to container" << endl;
	std::vector<Element*>&& re = c.Query({ 5, 4.5 });
	for (auto& iter : re) {
		EleA* ele = static_cast<EleA*>(iter);
		cout << (*ele) << endl;
	}
	re.clear();
	cout << "Call Element's SetPos Function" << endl;
	e.SetPos({ 2, 2 });
	re = c.Query({ 6.5, 6.5 });
	for (auto& iter : re) {
		EleA* ele = static_cast<EleA*>(iter);
		cout << (*ele) << endl;
	}
	re.clear();
	cout << "Call Element's SetSize Function" << endl;
	e.SetSize({ 3, 3 });
	re = c.Query({ 8, 8 });
	for (auto& iter : re) {
		EleA* ele = static_cast<EleA*>(iter);
		cout << (*ele) << endl;
	}
	re.clear();
	cout << "If query nothing" << endl;
	re = c.Query({ 1, 2 });
	for (auto& iter : re) {
		EleA* ele = static_cast<EleA*>(iter);
		cout << (*ele) << endl;
	}
	re.clear();
	system("pause");
}