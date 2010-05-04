/*
 * sorted-list.c
 *
 *
 */

#include <stdlib.h>
#include "sorted-list.h"

/** tree interface **/
typedef struct bstnode {
	void *data;
	struct bstnode *left;
	struct bstnode *right;
} node_t;

node_t *mk_node(void *data)
{
	struct bstnode *n = malloc(sizeof(*n));
	if (n) {
		n->data = data;
		n->left = n->right = NULL;
	}
	return n;
}

int insert_into_bst_smart(node_t **root, void *data,
		     CompareFuncT comparator, UnionFuncT uf)
{
	node_t **current = root;
	int compare;
	while (*current != NULL) {
		compare = comparator((*current)->data, data);
		if ( compare > 0 )
			current = &((*current)->right);
		else if ( compare < 0)
			current = &((*current)->left);
		else {
			uf( (*current)->data, data );
			return 0;
		}
	}
	*current = mk_node(data);
	return (*current ? 1 : 0);
}

int insert_into_bst(node_t **root, void *data,
		     CompareFuncT comparator)
{
	node_t **current = root;
	int compare;
	while (*current != NULL) {
		compare = comparator((*current)->data, data);
		if ( compare > 0 )
			current = &((*current)->right);
		else if ( compare < 0)
			current = &((*current)->left);
		else
			return 0;
	/*	(compare > 0) ? current = &((*current)->right) : (current =
								  &((*current)->left)); */
	}
	*current = mk_node(data);
	return (*current ? 1 : 0);
}

#if 0
int node_insert(node_t **root, node_t *in) {
	node_t **current = root;
	int compare;
	while (*current != NULL) {
		compare = comparator((*current)->data, in->data);
		if ( compare > 0 )
			current = &((*current)->right);
		else if ( compare < 0)
			current = &((*current)->left);
		else
			return 1;
	/*	(compare > 0) ? current = &((*current)->right) : (current =
								  &((*current)->left)); */
	}
	*current = in;
	return 0; 
}
#endif

node_t *delete_from_bst(node_t **root, void *data,
		    CompareFuncT comparator)
{
	node_t **current = root;
	node_t **previous = NULL;
	int compare;

	while (*current != NULL) {
		compare = comparator((*current)->data, data);

		if (compare == 0) {
			void *ret;
			if ((*current)->left == NULL
			    && (*current)->right == NULL) {
				ret = (*current)->data;
				free(*current);
				*current = NULL;
			} else if ((*current)->left == NULL) {
				node_t *temp = *current;
				*current = (*current)->right;
				ret = temp->data;
				free(temp);
			} else if ((*current)->right == NULL) {
				node_t *temp = *current;
				*current = (*current)->left;
				ret = temp->data;
				free(temp);
			} else {
				node_t *temp = (*current)->left;
				node_t *temp1 = (*current);
				(*current) = (*current)->right;
				ret = temp1->data;
				free(temp1);
				while (*current != NULL) {
					current = &((*current)->left);
				}
				(*current) = temp;
			}
			return ret;
		}
		previous = current;
		(compare > 0) ? current = &((*current)->right) : (current =
								  &((*current)->left));
	}
	return 0;
}


void *tree_lookup(node_t *root, void *data, CompareFuncT cmp) {
	if ( root ) {
		int c = cmp(root->data,data);
		if (c < 0) {
			return tree_lookup(root->left,data,cmp);
		} else if (c > 0) {
			return tree_lookup(root->right,data,cmp);
		} else {
			return root->data;
		}
	} else {
		return 0;
	}
}

void freebst(node_t *s)
{
	if (s == NULL)
		return;
	freebst(s->left);
	freebst(s->right);
	free(s);
	return;
}

/*                void *[]*current. must be large enough. */
void tree_flatten(void ***current, node_t *node)
{
	if (node == NULL)
		return;
	tree_flatten(current, node->left);

	**current = node->data;
	(*current)++;

	tree_flatten(current, node->right);
}

node_t *tree_dup(const node_t *t) {
	if (t) {
		node_t *n = mk_node(t->data);
		if (n) {
			n->left = tree_dup(t->left);
			n->right= tree_dup(t->right);
		}
		return n;
	} else {
		return 0;
	}
}

void tree_union_smart(node_t **d, node_t *s, size_t *n, CompareFuncT cmp, UnionFuncT uf) {
	if (s) {
		if (insert_into_bst_smart(d,s->data,cmp,uf))
			(*n)++;
		tree_union(d,s->left,n,cmp,uf);
		tree_union(d,s->right,n,cmp,uf);
	}
}

void tree_union(node_t **d, node_t *s, size_t *n, CompareFuncT cmp) {
	if (s) {
		if (insert_into_bst(d,s->data,cmp))
			(*n)++;
		tree_union(d,s->left,n,cmp);
		tree_union(d,s->right,n,cmp);
	}
}

void tree_intersect(node_t **d, node_t *sroot, size_t *n, CompareFuncT cmp) {
	if (*d) {
		if (!tree_lookup(sroot,(*d)->data,cmp)) {
			delete_from_bst(d,(*d)->data,cmp);
			(*n)--;
			tree_intersect(d,sroot,n,cmp);
		} else {
			tree_intersect(&((*d)->left) ,sroot,n,cmp);
			tree_intersect(&((*d)->right),sroot,n,cmp);
		}
	} 
}


/** sorted list interface **/
struct SortedList {
	struct bstnode *root;
	size_t ct;
	CompareFuncT cmp;

	pthread_rwlock_t rwlock;

	size_t iter_ct;
};

struct SortedListIterator {
	void **start;
	void **current;
	size_t len;
	SortedListPtr list;
};

SortedListPtr SLCreate(CompareFuncT cmp)
{
	SortedListPtr s = malloc(sizeof(*s));
	if (s) {
		s->root = NULL;
		s->ct = 0;
		s->cmp = cmp;
		s->iter_ct = 0;
		pthread_rwlock_init(s->rwlock,NULL);
	}
	return s;
}

void SLDestroy(SortedListPtr list)
{
	pthread_rwlock_destroy(s->rwlock);
	freebst(list->root);
	free(list);
}


int SLUnionSmart(SortedListPtr d, const SortedListPtr s, UnionFuncT uf) {

	pthread_rwlock_rdlock(s->rwlock);
	pthread_rwlock_wrlock(d->rwlock);

	tree_union_smart(&(d->root),s->root,&(d->ct),d->cmp,uf);
	
	pthread_rwlock_unlock(s->rwlock);
	pthread_rwlock_unlock(d->rwlock);

	return 1;
}

int SLUnion(SortedListPtr d, const SortedListPtr s) {
	/* 
	node_t *dn = tree_union(&(d->root),s->root);	
	if (dn) {
		d->root = dn;
		return 1;	
	}
	return 0;
	*/
	pthread_rwlock_rdlock(s->rwlock);
	pthread_rwlock_wrlock(d->rwlock);

	tree_union(&(d->root),s->root,&(d->ct),d->cmp);
	
	pthread_rwlock_unlock(s->rwlock);
	pthread_rwlock_unlock(d->rwlock);

	return 1;
}

int SLIntersect(SortedListPtr d, const SortedListPtr s) {
	/*
	SortedListIteratorPtr iter = SLCreateIterator(s);
	
	void *n;
	while((n = SLNextItem(iter))) {
		void *m = SLLookup(d,n);
		if (!m) {
			SLRemove(d,
		}

	}
	*/
	/*
	node_t *dn = tree_intersect(d->root,s->root);
	if (dn) {
		d->root = dn;
		return 1;
	}
	return 0;
	*/

	pthread_rwlock_rdlock(s->rwlock);
	pthread_rwlock_wrlock(d->rwlock);
	
	tree_intersect(&(d->root),s->root,&(d->ct),d->cmp);

	pthread_rwlock_unlock(s->rwlock);
	pthread_rwlock_unlock(d->rwlock);

	return 1;
}


SortedListPtr SLDup(SortedListPtr s) {
	if (s) {
		pthread_rwlock_rdlock(s->rwlock);

		SortedListPtr n = malloc(sizeof(*n));
		if (n) {
			n->root = tree_dup(s->root);
			n->ct = s->ct;
			n->cmp = s->cmp;
			n->iter_ct = 0;

			pthread_rwlock_init(n->rwlock,NULL);
		}
		pthread_rwlock_unlock(s->rwlock);
		return n;
	} else {
		return 0;
	}
}

void *SLLookup(SortedListPtr s, void *data) {
	void *ret;
	pthread_rwlock_rdlock(s->rwlock);
	ret = tree_lookup(s->root,data,s->cmp);
	pthread_rwlock_unlock(s->rwlock);
	return ret;
}

size_t SLGetCt(SortedListPtr list) {
	size_t ret;
	pthread_rwlock_rdlock(s->rwlock);
	ret = list->ct;
	pthread_rwlock_unlock(s->rwlock);
	return ret;
}

/* Block if the thread has open iterators. */
int SLInsert(SortedListPtr list, void *newObj)
{
	pthread_rwlock_wrlock(&list->rwlock);
	int ret;
	
	ret = insert_into_bst(&(list->root), newObj, list->cmp);
	if (ret)
		list->ct++;

	pthread_rwlock_unlock(list->rwlock);
	return ret;
}

/* Block if the thread has open iterators. */
int SLRemove(SortedListPtr list, void *newObj)
{
	node_t *ret;
	pthread_rwlock_wrlock(list->rwlock);
	ret = delete_from_bst(&(list->root), newObj, list->cmp);
	if (ret)
		list->ct--;
	pthread_rwlock_unlock(list->rwlock);
	return !!ret;
}

SortedListIteratorPtr SLCreateIterator(SortedListPtr list)
{
	SortedListIteratorPtr iter;

	pthread_rwlock_rdlock(list->rwlock);

	list->iter_ct++;
	iter = malloc(sizeof(*iter));
	iter->len = list->ct;
	iter->start = malloc(sizeof(void *) * iter->len);
	iter->current = iter->start;

	iter->list = list;

	/* Populate array */
	tree_flatten(&(iter->current), list->root);
	iter->current = iter->start;
	

	return iter;
}

void SLDestroyIterator(SortedListIteratorPtr iter)
{
	free(iter->start);
	pthread_rwlock_unlock(list->rwlock);
	pthread_rwlock_rwlock(list->rwlock);
	iter->list->iter_ct--;

	pthread_rwlock_unlock(list->rwlock);
	free(iter);
}

void *SLNextItem(SortedListIteratorPtr iter)
{
	void *tmp;

	if ((iter->current - iter->start) >= iter->len)
		return NULL;

	tmp = *(iter->current);
	iter->current++;
	return tmp;
}

