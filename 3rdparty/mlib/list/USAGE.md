# list

Typed doubly-linked list, header-only, plain ISO C11 (no compiler extensions).
Same generator pattern as [`../vec`](../vec).

`DEFINE_LIST(T, Name)` stamps out a node type, a `{ head, tail, count }`
container, and `static inline` `Name_*` functions for that one element type.

```c
#include "list.h"

DEFINE_LIST(int, IntList);   // note the trailing ';'

int main(void)
{
    IntList l = {0};                 // {0} is a valid empty list

    IntList_push_back(&l, 1);        // O(1)
    IntList_node* n = IntList_push_front(&l, 0);
    IntList_insert_after(&l, n, 9);  // 0, 9, 1

    list_foreach(IntList, it, &l)
        printf("%d ", it->value);

    IntList_erase(&l, n);            // O(1) given the node
    IntList_free(&l);                // frees every node
}
```

`Name` is caller-owned (stack or embedded); only nodes are heap allocated.
Values are copied into the node by assignment — if `T` owns resources, the
caller frees them before erasing.

## Generated API (for `DEFINE_LIST(T, Name)`)

| Function                                                       | Notes                                   |
| ------------------------------------------------------------- | --------------------------------------- |
| `Name##_node* Name##_push_front(Name* l, T v)`                 | O(1); returns the new node              |
| `Name##_node* Name##_push_back(Name* l, T v)`                  | O(1); returns the new node              |
| `bool Name##_pop_front(Name* l, T* out)`                       | copies to `*out` if non-NULL; false if empty |
| `bool Name##_pop_back(Name* l, T* out)`                        | "                                       |
| `Name##_node* Name##_insert_after(Name* l, Name##_node* at, T v)`  | `at` must be in `l`                  |
| `Name##_node* Name##_insert_before(Name* l, Name##_node* at, T v)` | "                                   |
| `void Name##_erase(Name* l, Name##_node* n)`                   | O(1); unlink + free `n`                  |
| `Name##_node* Name##_at(Name* l, size_t i)`                    | O(n), walks from the nearer end; NULL if out of range |
| `bool Name##_remove_at(Name* l, size_t i)`                     | false if out of range                   |
| `Name##_node* Name##_find_if(Name* l, bool (*pred)(const T*, void*), void* ctx)` | first match, else NULL |
| `void Name##_clear(Name* l)`                                   | free every node; list stays usable      |
| `void Name##_free(Name* l)`                                    | `clear`, and `NULL`-safe                 |

Iteration macros (top-level, `Name` is the list type):

```c
list_foreach(IntList, it, &l)     { use(it->value); }   // head -> tail
list_foreach_rev(IntList, it, &l) { use(it->value); }   // tail -> head
```

## Tuning

Define before including (defaults shown):

```c
#define LIST_ASSERT assert
#define LIST_MALLOC malloc
#define LIST_FREE   free
#include "list.h"
```

## Tests & examples

```
make            # build and run test/test_list.c
make examples   # build and run example/list.c
make clean
```

On Windows use `mingw32-make`. The test suite uses the repo-wide harness `test.h`
at the repository root (`-I..`).
