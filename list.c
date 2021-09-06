#include "list.h"


int list_empty(struct list_head *lst) {
    return lst == lst->next;
}

// adds the element at the end.
void list_add(struct list_head *lst, struct list_head *e) {
  e->next = lst;
  e->prev = lst->prev;
  lst->prev->next = e;
  lst->prev = e;
}

void list_addAfter(struct list_head *b, struct list_head *e) {
    e->next = b->next;
    e->prev = b;
    b->next->prev = e;
    b->next = e;
}

void list_deattach(struct list_head *elem) {
  elem->prev->next = elem->next;
  elem->next->prev = elem->prev;
}

