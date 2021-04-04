#include "list.h"

void list_add(struct list_head *lst, struct list_head *e) {
  e->next = lst;
  e->prev = lst->prev;
  lst->prev->next = e;
  lst->prev = e;
}

void list_deattach(struct list_head *elem) {
  elem->prev->next = elem->next;
  elem->next->prev = elem->prev;
}

