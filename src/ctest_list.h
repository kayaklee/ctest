#ifndef CTEST_LIST_H_
#define CTEST_LIST_H_

/**
 * 列表，参考kernel上的list.h
 */
#include "ctest_define.h"

CTEST_CPP_START

// from kernel list
typedef struct ctest_list_t ctest_list_t;

struct ctest_list_t {
    ctest_list_t             *next, *prev;
};

#define CTEST_LIST_HEAD_INIT(name) {&(name), &(name)}
#define ctest_list_init(ptr) do {                \
        (ptr)->next = (ptr);                    \
        (ptr)->prev = (ptr);                    \
    } while (0)

static inline void __ctest_list_add(ctest_list_t *list,
                                   ctest_list_t *prev, ctest_list_t *next)
{
    next->prev = list;
    list->next = next;
    list->prev = prev;
    prev->next = list;
}
// list head to add it after
static inline void ctest_list_add_head(ctest_list_t *list, ctest_list_t *head)
{
    __ctest_list_add(list, head, head->next);
}
// list head to add it before
static inline void ctest_list_add_tail(ctest_list_t *list, ctest_list_t *head)
{
    __ctest_list_add(list, head->prev, head);
}
static inline void __ctest_list_del(ctest_list_t *prev, ctest_list_t *next)
{
    next->prev = prev;
    prev->next = next;
}
// deletes entry from list
static inline void ctest_list_del(ctest_list_t *entry)
{
    __ctest_list_del(entry->prev, entry->next);
    ctest_list_init(entry);
}
// tests whether a list is empty
static inline int ctest_list_empty(const ctest_list_t *head)
{
    return (head->next == head);
}
// move list to new_list
static inline void ctest_list_movelist(ctest_list_t *list, ctest_list_t *new_list)
{
    if (!ctest_list_empty(list)) {
        new_list->prev = list->prev;
        new_list->next = list->next;
        new_list->prev->next = new_list;
        new_list->next->prev = new_list;
        ctest_list_init(list);
    } else {
        ctest_list_init(new_list);
    }
}
// join list to head
static inline void ctest_list_join(ctest_list_t *list, ctest_list_t *head)
{
    if (!ctest_list_empty(list)) {
        ctest_list_t             *first = list->next;
        ctest_list_t             *last = list->prev;
        ctest_list_t             *at = head->prev;

        first->prev = at;
        at->next = first;
        last->next = head;
        head->prev = last;
    }
}

// get last
#define ctest_list_get_last(list, type, member)                              \
    ctest_list_empty(list) ? NULL : ctest_list_entry((list)->prev, type, member)

// get first
#define ctest_list_get_first(list, type, member)                             \
    ctest_list_empty(list) ? NULL : ctest_list_entry((list)->next, type, member)

#define ctest_list_entry(ptr, type, member) ({                               \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);                \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#define ctest_list_for_each_entry(pos, head, member)                         \
    for (pos = ctest_list_entry((head)->next, typeof(*pos), member);         \
            &pos->member != (head);                                         \
            pos = ctest_list_entry(pos->member.next, typeof(*pos), member))

#define ctest_list_for_each_entry_reverse(pos, head, member)                 \
    for (pos = ctest_list_entry((head)->prev, typeof(*pos), member);     \
            &pos->member != (head);                                        \
            pos = ctest_list_entry(pos->member.prev, typeof(*pos), member))

#define ctest_list_for_each_entry_safe(pos, n, head, member)                 \
    for (pos = ctest_list_entry((head)->next, typeof(*pos), member),         \
            n = ctest_list_entry(pos->member.next, typeof(*pos), member);    \
            &pos->member != (head);                                         \
            pos = n, n = ctest_list_entry(n->member.next, typeof(*n), member))

#define ctest_list_for_each_entry_safe_reverse(pos, n, head, member)         \
    for (pos = ctest_list_entry((head)->prev, typeof(*pos), member),         \
            n = ctest_list_entry(pos->member.prev, typeof(*pos), member);    \
            &pos->member != (head);                                         \
            pos = n, n = ctest_list_entry(n->member.prev, typeof(*n), member))

CTEST_CPP_END

#endif
