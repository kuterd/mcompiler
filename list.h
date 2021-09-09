// Kuter Dinel [UNKNOW DATE]
// Linux style intrusive linked list.
#ifndef LIST_H
#define LIST_H

#include "utils.h"

struct list_head {
    struct list_head *prev, *next;
};

#define LIST_INIT(l) (l)->prev = (l)->next = (l);
#define LIST_FOR_EACH(l)                                                       \
    for (struct list_head *c = (l)->next; c != (l); c = c->next)

int list_empty(struct list_head *lst);
void list_add(struct list_head *lst, struct list_head *e);
void list_deattach(struct list_head *elem);
void list_addAfter(struct list_head *b, struct list_head *e);

#endif
