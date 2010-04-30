#ifndef SORTED_LIST_H_
#define SORTED_LIST_H_

/* a > b -> < 0
 * a < b -> > 0 
 * a = b -> = 0
 */
typedef int (*CompareFuncT) (void *a, void *b);
typedef struct SortedList *SortedListPtr;
typedef struct SortedListIterator *SortedListIteratorPtr;

SortedListPtr SLCreate(CompareFuncT cf);
SortedListPtr SLDup(SortedListPtr s);
int SLUnion(SortedListPtr d, const SortedListPtr s);
int SLIntersect(SortedListPtr d, const SortedListPtr s);
void *SLLookup(SortedListPtr s, void *data);
void SLDestroy(SortedListPtr list);
int SLInsert(SortedListPtr list, void *newObj);
int SLRemove(SortedListPtr list, void *newObj);



SortedListIteratorPtr SLCreateIterator(SortedListPtr list);
void SLDestroyIterator(SortedListIteratorPtr iter);
void *SLNextItem(SortedListIteratorPtr iter);

#endif
