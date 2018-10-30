#ifndef LINKLIST_H
#define LINKLIST_H

template<class TYPE> class List;

template<class TYPE>
class ListItem {
public:
	TYPE *previous, *next;
	List<TYPE> *list;

	int get_item_number() { return !list ? -1 : list->number_of(this); }
	ListItem() { list = 0;  previous = next = 0; }
	ListItem(List<TYPE> &me) { list = me;  previous = next = 0; }
	virtual ~ListItem() { if( list ) list->remove_pointer(this); }
};

template<class TYPE>
class List {
	TYPE *split(int (*cmpr)(TYPE *a, TYPE *b),TYPE *l, TYPE *r);
	static int cmpr(TYPE *a, TYPE *b) {
		if( *a == *b ) return 0;
		return *a > *b ? 1 : -1;
	}
public:
	TYPE *first, *last;
	void remove(TYPE *item) { if(item) delete item; }
	void remove_pointer(ListItem<TYPE> *item);
	TYPE *append(TYPE *new_item);
	TYPE *append() { return append(new TYPE()); }
	TYPE *insert_before(TYPE *here, TYPE *item);
	TYPE *insert_before(TYPE *here) { return insert_before(here, new TYPE()); }
	TYPE *insert_after(TYPE *here, TYPE *item);
	TYPE *insert_after(TYPE *here) { return insert_after(here, new TYPE()); }
	int total() { TYPE *p=first; int n=0; while(p) { p=p->next; ++n; } return n; }
	TYPE *get_item_number(int i) { TYPE *p=first;
		while(p && i>0) { --i; p=p->next; }
		return p;
	}
	TYPE &operator[](int i) { return *get_item_number(i); }
	int number_of(TYPE *item) { TYPE *p=first; int i=0;
		while(p && p!=item) { ++i; p=p->next; }
		return p ? i : -1;
	}
	void swap(TYPE *item1, TYPE *item2);
	void sort(TYPE *ap=0, TYPE *bp=0) { return sort(cmpr,ap,bp); }
	void sort(int (*cmp)(TYPE *a, TYPE *b), TYPE *ap=0, TYPE *bp=0);
	List() { first = last = 0; }
	virtual ~List() { while(last) delete last; }
};

// convenience macros
#define PREVIOUS current->previous
#define NEXT current->next

template<class TYPE>
TYPE* List<TYPE>::append(TYPE *item)
{
	item->list = this;  item->next = 0;
	if( !last ) { item->previous = 0; first = item; }
	else { item->previous = last;  last->next = item; }
	return last = item;
}

template<class TYPE>
TYPE* List<TYPE>::insert_before(TYPE *here, TYPE *item)
{
	if( !here || !last ) return append(item);
	item->list = this;  item->next = here;
	*( !(item->previous=here->previous) ? &first : &here->previous->next ) = item;
	return here->previous = item;
}

template<class TYPE>
TYPE* List<TYPE>::insert_after(TYPE *here, TYPE *item)
{
	if( !here || !last ) return append(item);
	item->list = this;  item->previous = here;
	*( !(item->next=here->next) ? &last : &here->next->previous ) = item;
	return here->next = item;
}


template<class TYPE>
void List<TYPE>::remove_pointer(ListItem<TYPE> *item)
{
	if( !item ) return;
	TYPE *previous = item->previous, *next = item->next;
	*( previous ? &previous->next : &first ) = next;
	*( next ? &next->previous : &last ) = previous;
	item->list = 0;
}

template<class TYPE>
void List<TYPE>::swap(TYPE *item1, TYPE *item2)
{
	if( item1 == item2 ) return;
	TYPE *item1_previous = item1->previous, *item1_next = item1->next;
	TYPE *item2_previous = item2->previous, *item2_next = item2->next;
	*( item1_previous ? &item1_previous->next : &first ) = item2;
	*( item1_next     ? &item1_next->previous : &last  ) = item2;
	*( item2_previous ? &item2_previous->next : &first ) = item1;
	*( item2_next     ? &item2_next->previous : &last  ) = item1;
	item1->previous = item2_previous == item1 ? item2 : item2_previous;
	item1->next     = item2_next     == item1 ? item2 : item2_next;
	item2->previous = item1_previous == item2 ? item1 : item1_previous;
	item2->next     = item1_next     == item2 ? item1 : item1_next;
}

template<class TYPE>
TYPE *List<TYPE>::split(int (*cmpr)(TYPE *a, TYPE *b), TYPE *l, TYPE *r)
{
	for(;;) {
		while( cmpr(r,l) >= 0 ) if( (l=l->next) == r ) return r;
		swap(l, r);
		while( cmpr(l,r) >= 0 ) if( r == (l=l->previous) ) return r;
		swap(l, r);
	}
}

template<class TYPE>
void List<TYPE>::sort(int (*cmpr)(TYPE *a, TYPE *b), TYPE *ll, TYPE *rr)
{
	for(;;) {
		TYPE *l = !ll ? first : ll->next;
		if( l == rr ) return;
		TYPE *r = !rr ? last  : rr->previous;
		if( l == r ) return;
		TYPE *m = split(cmpr, l, r);
		sort(cmpr, ll, m);
		ll = m;
	}
}

#endif
