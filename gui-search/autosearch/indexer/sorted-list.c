/*
 * sorted-list.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
/*
 * When your sorted list is used to store objects of some type, since the
 * type is opaque to you, you will need a comparator function to order
 * the objects in your sorted list.
 *
 * You can expect a comparator function to return -1 if the 1st object is
 * smaller, 0 if the two objects are equal, and 1 if the 2nd object is
 * smaller.
 *
 * Note that you are not expected to implement any comparator functions.
 * You will be given a comparator function when a new sorted list is
 * created.
 */
typedef int (*CompareFuncT) (void *, void *);

/*
 *  This struct and associated functions are used to operate on the
 *  binary search tree structure.
 */

struct bstnode {
	void *data;
	struct bstnode *left;
	struct bstnode *right;
};
struct bstnode *newnode(void *data);
int insert_into_bst(struct bstnode **root, void *data,
		     CompareFuncT comparator);
int delete_from_bst(struct bstnode **root, void *data,
		    CompareFuncT comparator);
void freebst(struct bstnode *s);

/*
 * Sorted list type.  You need to fill in the type as part of your implementation.
 */
struct SortedList {
	struct bstnode *root;
	size_t ct;
	CompareFuncT cf;
	#ifndef REMOVE_SILLYNESS
	size_t iter_ct;
	#endif
};
typedef struct SortedList *SortedListPtr;

/*
 * Iterator type for user to "walk" through the list item by item, from
 * beginning to end.  You need to fill in the type as part of your implementation.
 */
struct SortedListIterator {
	void **start;
	void **current;
	size_t len;
	#ifndef REMOVE_SILLYNESS
	/* W/SILLYNESS we waste memory by allocating for each iterator. */
	SortedListPtr list;
	#endif
};
typedef struct SortedListIterator *SortedListIteratorPtr;

/*
 * SLCreate creates a new, empty sorted list.  The caller must provide
 * a comparator function that can be used to order objects that will be
 * kept in the list.
 *
 * If the function succeeds, it returns a (non-NULL) SortedListT object.
 * Else, it returns NULL.
 *
 * You need to fill in this function as part of your implementation.
 */
SortedListPtr SLCreate(CompareFuncT cf)
{
	SortedListPtr s = malloc(sizeof(struct SortedList));
	s->root = NULL;
	s->ct = 0;
	s->cf = cf;
	#ifndef REMOVE_SILLYNESS
	s->iter_ct = 0;
	#endif
	return s;
}

/*
 * SLDestroy destroys a list, freeing all dynamically allocated memory.
 * Ojects should NOT be deallocated, however.  That is the responsibility
 * of the user of the list.
 *
 * You need to fill in this function as part of your implementation.
 */
void SLDestroy(SortedListPtr list)
{
	#ifndef REMOVE_SILLYNESS
	/* well, what if we destroy a list with iterators? HUH!?! */
	#endif
	freebst(list->root);
	free(list);
}

/*
 * SLInsert inserts a given object into a sorted list, maintaining sorted
 * order of all objects in the list.  If the new object is equal to a subset
 * of existing objects in the list, then the subset can be kept in any
 * order.
 *
 * If the function succeeds, it returns 1.  Else, it returns 0.
 *
 * You need to fill in this function as part of your implementation.
 */
int SLInsert(SortedListPtr list, void *newObj)
{
	int ret;
	#ifndef REMOVE_SILLYNESS
	if (list->iter_ct)
		return 0;
	#endif
	ret = insert_into_bst(&(list->root), newObj, list->cf);
	if (ret)
		list->ct++;
	return ret;
}

/*
 * SLRemove removes a given object from a sorted list.  Sorted ordering
 * should be maintained.
 *
 * If the function succeeds, it returns 1.  Else, it returns 0.
 *
 * You need to fill in this function as part of your implementation.
 */
int SLRemove(SortedListPtr list, void *newObj)
{
	int ret;
	#ifndef REMOVE_SILLYNESS
	if (list->iter_ct)
		return 0;
	#endif
	ret = delete_from_bst(&(list->root), newObj, list->cf);
	if (ret)
		list->ct--;
	return ret;
}

/*
 * SLCreateIterator creates an iterator object that will allow the caller
 * to "walk" through the list from beginning to the end using SLNextItem.
 *
 * If the function succeeds, it returns a non-NULL SortedListIterT object.
 * Else, it returns NULL.
 *
 * You need to fill in this function as part of your implementation.
 */
void store_iter(void ***current, struct bstnode *node)
{
	if (node == NULL)
		return;
	store_iter(current, node->left);

	**current = node->data;
	(*current)++;

	store_iter(current, node->right);
}

SortedListIteratorPtr SLCreateIterator(SortedListPtr list)
{
	SortedListIteratorPtr iter;
	#ifndef REMOVE_SILLYNESS
	list->iter_ct++;
	#endif
	iter = malloc(sizeof(*iter));
	iter->len = list->ct;
	iter->start = malloc(sizeof(void *) * iter->len);
	iter->current = iter->start;

	#ifndef REMOVE_SILLYNESS
	iter->list = list;
	#endif

	/* Populate array */
	store_iter(&(iter->current), list->root);
	iter->current = iter->start;

	return iter;
}

/*
 * SLDestroyIterator destroys an iterator object that was created using
 * SLCreateIterator().  Note that this function should destroy the
 * iterator but should NOT affectt the original list used to create
 * the iterator in any way.
 *
 * You need to fill in this function as part of your implementation.
 */
void SLDestroyIterator(SortedListIteratorPtr iter)
{
	free(iter->start);
	#ifndef REMOVE_SILLYNESS
	iter->list->iter_ct--;
	#endif
	free(iter);
}


/*
 * SLNextItem returns the next object in the list encapsulated by the
 * given iterator.  It should return a NULL when the end of the list
 * has been reached.
 *
 * One complication you MUST consider/address is what happens if a
 * sorted list encapsulated within an iterator is modified while that
 * iterator is active.  For example, what if an iterator is "pointing"
 * to some object in the list as the next one to be returned but that
 * object is removed from the list using SLRemove() before SLNextItem()
 * is called.
 * >>>Response: The iterator will only see the elements that were present
 *              during it's creation, no more, no less. In order for the 
 *              list to be modified, it must not have any existing 
 *              iterators.
 *
 *
 * You need to fill in this function as part of your implementation.
 */
void *SLNextItem(SortedListIteratorPtr iter)
{
	void *tmp;

	if ((iter->current - iter->start) >= iter->len)
		return NULL;

	tmp = *(iter->current);
	iter->current++;
	return tmp;
}

/*
 * Binary Search Tree Implementation
 */
struct bstnode *newnode(void *data)
{
	struct bstnode *n = malloc(sizeof(*n));
	if (n) {
		n->data = data;
		n->left = n->right = NULL;
	}
	return n;
}

int insert_into_bst(struct bstnode **root, void *data,
		     CompareFuncT comparator)
{
	struct bstnode **current = root;
	int compare;
	while (*current != NULL) {
		compare = comparator((*current)->data, data);
		(compare > 0) ? current = &((*current)->right) : (current =
								  &((*current)->left));
	}
	*current = newnode(data);
	return (*current ? 1 : 0);
}

int delete_from_bst(struct bstnode **root, void *data,
		    CompareFuncT comparator)
{
	struct bstnode **current = root;
	struct bstnode **previous = NULL;
	struct bstnode *temp;
	struct bstnode *temp1;
	int compare;

	while (*current != NULL) {
		compare = comparator((*current)->data, data);

		if (compare == 0) {
			if ((*current)->left == NULL
			    && (*current)->right == NULL) {
				free(*current);
				*current = NULL;
			} else if ((*current)->left == NULL) {
				temp = *current;
				*current = (*current)->right;
				free(temp);
			} else if ((*current)->right == NULL) {
				temp = *current;
				*current = (*current)->left;
				free(temp);
			} else {
				temp = (*current)->left;
				temp1 = (*current);
				(*current) = (*current)->right;
				free(temp1);
				while (*current != NULL) {
					current = &((*current)->left);
				}
				(*current) = temp;
			}
			return 1;
		}
		previous = current;
		(compare > 0) ? current = &((*current)->right) : (current =
								  &((*current)->left));
	}
	return 0;
}

void freebst(struct bstnode *s)
{
	if (s == NULL)
		return;
	freebst(s->left);
	freebst(s->right);
	free(s);
	return;
}
