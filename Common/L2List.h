#ifndef _L2LIST_H
#define _L2LIST_H

#include <stdlib.h>

class ListElem
{
private:
	ListElem *prev, *next;

	friend class List;

public:
	virtual ~ListElem(void) {}
	ListElem(void): prev(this), next(this) {}

	void Link(ListElem *el) { next = el; el->prev = this; }
	void BackLink(ListElem *el) { prev = el; el->next = this; }
};

class List
{
private:
	ListElem beg, end;
	ListElem *cur;

public:
	~List() { GotoFirst(); while (Remove(true)); }
	List(): beg(), end(), cur(&beg) { beg.Link(&end); }

	bool Remove(bool go_to_next);
	void AddAfter(ListElem* element);
	void AddBefore(ListElem* element);

	bool OnElement(void) { return (cur != &beg && cur != &end); }

	bool GotoLast(void) { cur = end.prev; return (cur != &beg); }
	bool GotoFirst(void) { cur = beg.next; return (cur != &end); }
	bool GotoNext(void) { cur = cur->next; return (cur != &end); }
	bool GotoPrev(void) { cur = cur->prev; return (cur != &beg); }

	ListElem* GetCur(void) { if (OnElement()) return cur; return NULL;}
	ListElem* GetPrev(void) { if (cur->prev != &beg) return cur->prev; return NULL; }
	ListElem* GetNext(void) { if (cur->next != &end) return cur->next; return NULL; }
};

inline bool List::Remove(bool goNext)
{
	ListElem* del = cur;
	if (!OnElement())
		return false;

	cur->prev->Link(cur->next);
	cur = (goNext ? cur->next : cur->prev);

	delete del;
	return OnElement();
}

inline void List::AddAfter(ListElem* el)
{
	if (cur == &end)
		AddBefore(el);
	else
	{
		el->Link(cur->next);
		el->BackLink(cur);
	}
}

inline void List::AddBefore(ListElem* el)
{
	if (cur == &beg)
		AddAfter(el);
	else
	{
		el->BackLink(cur->prev);
		el->Link(cur);
	}
}

#endif
