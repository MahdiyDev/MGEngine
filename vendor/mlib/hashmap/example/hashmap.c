#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "../hashmap.h"

DEFINE_HASHMAP_STR(int, WordCount);   // const char* -> int, keys owned

typedef struct {
    int wins;
    int losses;
} Record;

DEFINE_HASHMAP_INT(int, Record, TeamMap);   // int -> Record

int main(void)
{
    // --- word frequency count -------------------------------------------
    const char* text =
        "the quick brown fox the lazy dog the fox jumps the dog sleeps the";

    WordCount wc = {0};
    char word[32];
    size_t w = 0;
    for (const char* p = text;; p++) {
        if (isalpha((unsigned char)*p)) {
            if (w < sizeof(word) - 1) {
                word[w++] = *p;
            }
        } else if (w > 0) {
            word[w] = '\0';
            (*WordCount_get_or_put(&wc, word, 0))++;
            w = 0;
        }
        if (*p == '\0') {
            break;
        }
    }

    printf("word counts (%zu distinct):\n", wc.count);
    hashmap_foreach(WordCount, it, &wc) {
        printf("  %-6s %d\n", it.key, *it.value);
    }

    printf("\"the\" appears %d times\n", *WordCount_get(&wc, "the"));
    WordCount_remove(&wc, "the");
    printf("after remove, contains \"the\": %s\n\n",
           WordCount_contains(&wc, "the") ? "yes" : "no");
    WordCount_free(&wc);

    // --- struct values, integer keys ----------------------------------
    TeamMap teams = {0};
    TeamMap_put(&teams, 7, (Record){ .wins = 0, .losses = 0 });
    TeamMap_get(&teams, 7)->wins += 3;
    TeamMap_get(&teams, 7)->losses += 1;
    TeamMap_put(&teams, 12, (Record){ .wins = 5, .losses = 5 });

    hashmap_foreach(TeamMap, it, &teams) {
        printf("team %d: %d-%d\n", it.key, it.value->wins, it.value->losses);
    }
    TeamMap_free(&teams);

    return 0;
}
