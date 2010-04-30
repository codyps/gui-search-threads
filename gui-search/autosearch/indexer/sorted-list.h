#ifndef SORTED_LIST_H_
#define SORTED_LIST_H_

typedef int (*CompareFuncT) (void *, void *);
typedef struct SortedList *SortedListPtr;
typedef struct SortedListIterator *SortedListIteratorPtr;

SortedListPtr SLCreate(CompareFuncT cf);
void SLDestroy(SortedListPtr list);
int SLInsert(SortedListPtr list, void *newObj);
int SLRemove(SortedListPtr list, void *newObj);

SortedListIteratorPtr SLCreateIterator(SortedListPtr list);
void SLDestroyIterator(SortedListIteratorPtr iter);
void *SLNextItem(SortedListIteratorPtr iter);

#endif
