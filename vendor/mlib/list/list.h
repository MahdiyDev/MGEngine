#pragma once

// Typed doubly-linked list, "C++ template" style -- the same generator pattern
// as vec.h. Every call is type checked, steps through in a debugger, and nothing
// here needs typeof / C23.
//
//     DEFINE_LIST(int, IntList);            // note the trailing ';'
//
//     IntList l = {0};                      // {0} is a valid empty list
//     IntList_push_back(&l, 1);             // O(1) at either end
//     IntList_node* n = IntList_push_front(&l, 0);
//     IntList_insert_after(&l, n, 9);       // 0, 9, 1
//
//     list_foreach(IntList, it, &l)
//         printf("%d ", it->value);
//
//     IntList_erase(&l, n);                 // O(1) given the node
//     IntList_free(&l);                     // frees every node
//
// DEFINE_LIST(T, Name) generates:
//   struct Name##_node { T value; Name##_node *prev, *next; }
//   struct Name        { Name##_node *head, *tail; size_t count; }
//   static inline Name##_* functions (see below)
//
// The Name value is caller-owned (stack or embedded); only the nodes are heap
// allocated. Name##_free / Name##_clear release every node. Values are copied
// into the node by assignment -- if T owns resources, the caller manages them.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#ifndef LIST_ASSERT
    #define LIST_ASSERT assert
#endif
#ifndef LIST_MALLOC
    #define LIST_MALLOC malloc
#endif
#ifndef LIST_FREE
    #define LIST_FREE free
#endif

// Iterate a generated list; `Name` is the type passed to DEFINE_LIST.
//     list_foreach(IntList, it, &l) { use(it->value); }
#define list_foreach(Name, it, lp) \
    for (Name##_node* it = (lp)->head; it != NULL; it = it->next)
#define list_foreach_rev(Name, it, lp) \
    for (Name##_node* it = (lp)->tail; it != NULL; it = it->prev)

#define DEFINE_LIST(T, Name) \
    typedef struct Name##_node { \
        T value; \
        struct Name##_node* prev; \
        struct Name##_node* next; \
    } Name##_node; \
 \
    typedef struct { \
        Name##_node* head; \
        Name##_node* tail; \
        size_t count; \
    } Name; \
 \
    static inline Name##_node* Name##_node_new_(T value) \
    { \
        Name##_node* n = (Name##_node*)LIST_MALLOC(sizeof(Name##_node)); \
        LIST_ASSERT(n != NULL && "Failed to allocate list node"); \
        n->value = value; \
        n->prev = NULL; \
        n->next = NULL; \
        return n; \
    } \
 \
    /* Insert before head; O(1). Returns the new node. */ \
    static inline Name##_node* Name##_push_front(Name* l, T value) \
    { \
        Name##_node* n = Name##_node_new_(value); \
        n->next = l->head; \
        if (l->head != NULL) { \
            l->head->prev = n; \
        } else { \
            l->tail = n; \
        } \
        l->head = n; \
        l->count++; \
        return n; \
    } \
 \
    /* Insert after tail; O(1). Returns the new node. */ \
    static inline Name##_node* Name##_push_back(Name* l, T value) \
    { \
        Name##_node* n = Name##_node_new_(value); \
        n->prev = l->tail; \
        if (l->tail != NULL) { \
            l->tail->next = n; \
        } else { \
            l->head = n; \
        } \
        l->tail = n; \
        l->count++; \
        return n; \
    } \
 \
    static inline Name##_node* Name##_insert_after(Name* l, Name##_node* at, T value) \
    { \
        LIST_ASSERT(at != NULL && "insert_after: NULL node"); \
        if (at == l->tail) { \
            return Name##_push_back(l, value); \
        } \
        Name##_node* n = Name##_node_new_(value); \
        n->prev = at; \
        n->next = at->next; \
        at->next->prev = n; \
        at->next = n; \
        l->count++; \
        return n; \
    } \
 \
    static inline Name##_node* Name##_insert_before(Name* l, Name##_node* at, T value) \
    { \
        LIST_ASSERT(at != NULL && "insert_before: NULL node"); \
        if (at == l->head) { \
            return Name##_push_front(l, value); \
        } \
        Name##_node* n = Name##_node_new_(value); \
        n->next = at; \
        n->prev = at->prev; \
        at->prev->next = n; \
        at->prev = n; \
        l->count++; \
        return n; \
    } \
 \
    /* Unlink and free `n`; O(1). `n` must belong to `l`. */ \
    static inline void Name##_erase(Name* l, Name##_node* n) \
    { \
        LIST_ASSERT(n != NULL && "erase: NULL node"); \
        LIST_ASSERT(l->count > 0 && "erase: empty list"); \
        if (n->prev != NULL) { \
            n->prev->next = n->next; \
        } else { \
            l->head = n->next; \
        } \
        if (n->next != NULL) { \
            n->next->prev = n->prev; \
        } else { \
            l->tail = n->prev; \
        } \
        LIST_FREE(n); \
        l->count--; \
    } \
 \
    /* Copy the front value to *out (if out != NULL), remove it. False if empty. */ \
    static inline bool Name##_pop_front(Name* l, T* out) \
    { \
        if (l->head == NULL) { \
            return false; \
        } \
        if (out != NULL) { \
            *out = l->head->value; \
        } \
        Name##_erase(l, l->head); \
        return true; \
    } \
 \
    static inline bool Name##_pop_back(Name* l, T* out) \
    { \
        if (l->tail == NULL) { \
            return false; \
        } \
        if (out != NULL) { \
            *out = l->tail->value; \
        } \
        Name##_erase(l, l->tail); \
        return true; \
    } \
 \
    /* Node at `index`, walked from the nearer end; NULL if out of range. */ \
    static inline Name##_node* Name##_at(Name* l, size_t index) \
    { \
        if (index >= l->count) { \
            return NULL; \
        } \
        Name##_node* n; \
        if (index <= l->count / 2) { \
            n = l->head; \
            for (size_t i = 0; i < index; i++) { \
                n = n->next; \
            } \
        } else { \
            n = l->tail; \
            for (size_t i = l->count - 1; i > index; i--) { \
                n = n->prev; \
            } \
        } \
        return n; \
    } \
 \
    static inline bool Name##_remove_at(Name* l, size_t index) \
    { \
        Name##_node* n = Name##_at(l, index); \
        if (n == NULL) { \
            return false; \
        } \
        Name##_erase(l, n); \
        return true; \
    } \
 \
    /* First node whose value satisfies `pred`, else NULL. */ \
    static inline Name##_node* Name##_find_if(Name* l, bool (*pred)(const T*, void*), void* ctx) \
    { \
        for (Name##_node* n = l->head; n != NULL; n = n->next) { \
            if (pred(&n->value, ctx)) { \
                return n; \
            } \
        } \
        return NULL; \
    } \
 \
    /* Free every node; the list becomes empty and is reusable. */ \
    static inline void Name##_clear(Name* l) \
    { \
        Name##_node* n = l->head; \
        while (n != NULL) { \
            Name##_node* next = n->next; \
            LIST_FREE(n); \
            n = next; \
        } \
        l->head = NULL; \
        l->tail = NULL; \
        l->count = 0; \
    } \
 \
    static inline void Name##_free(Name* l) \
    { \
        if (l != NULL) { \
            Name##_clear(l); \
        } \
    } \
 \
    /* consume the trailing semicolon at the call site */ \
    struct Name##_semicolon_eater_
