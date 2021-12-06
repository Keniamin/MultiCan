#ifndef _UNDOLIST_H
#define _UNDOLIST_H

#include <stdlib.h>
#include "L2List.h"

class UndoListElem: public ListElem
{
public:
	virtual ~UndoListElem(void) {};
	virtual void Apply(void) = 0;
};

class UndoList
{
private:
	List list;

public:
	~UndoList() {}
	UndoList(): list() {}

	void Undo(void);
	void Redo(void);
	void Clear(void);
	void Add(UndoListElem *operation);

	bool CanRedo(void) { return (list.GetCur() != NULL); }
	bool CanUndo(void) { return (list.GetNext() != NULL); }

	const UndoListElem* GetRedo(void) { return ((UndoListElem*) list.GetCur()); }
	const UndoListElem* GetUndo(void) { return ((UndoListElem*) list.GetNext()); }
};

inline void UndoList::Undo(void)
{
	UndoListElem *op;
	if ((op = (UndoListElem*) list.GetNext()))
	{
		op->Apply();
		list.GotoNext();
	}
}

inline void UndoList::Redo(void)
{
	UndoListElem *op;
	if ((op = (UndoListElem*) list.GetCur()))
	{
		op->Apply();
		list.GotoPrev();
	}
}

inline void UndoList::Clear(void)
{
	list.GotoFirst();
	while (list.Remove(true));
	list.GotoPrev();
}

inline void UndoList::Add(UndoListElem *operation)
{
	while (list.Remove(false));
	list.AddAfter(operation);
}

#endif
