#ifndef SORTED_LIST_H_
#define SORTED_LIST_H_

/* a > b -> < 0
 * a < b -> > 0 
 * a = b -> = 0
 */
typedef int (*CompareFuncT) (void *a, void *b);
typedef void (*UnionFuncT) (void *d, void *s); /* destination <- source */

typedef struct SortedList *SortedListPtr;
typedef struct SortedListIterator *SortedListIteratorPtr;

SortedListPtr SLCreate(CompareFuncT cf);
SortedListPtr SLDup(SortedListPtr s);
int SLUnion(SortedListPtr d, const SortedListPtr s);
int SLUnionSmart(SortedListPtr d, const SortedListPtr s, UnionFuncT uf);

int SLIntersect(SortedListPtr d, const SortedListPtr s);
void *SLLookup(SortedListPtr s, void *data);
void SLDestroy(SortedListPtr list);
int SLInsert(SortedListPtr list, void *newObj);
int SLRemove(SortedListPtr list, void *newObj);
size_t SLGetCt(SortedListPtr list);


SortedListIteratorPtr SLCreateIterator(SortedListPtr list);
void SLDestroyIterator(SortedListIteratorPtr iter);
void *SLNextItem(SortedListIteratorPtr iter);

#endif
