#ifndef reader_h_
#define reader_h_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <utility>

#define PANIC() do { perror(nullptr); exit(1); } while (0)

static inline
std::pair<const char *, size_t>
read_text(FILE *f)
{
    char *buf = nullptr;
    size_t size = 0;
    size_t capacity = 0;
    while (true) {
        if (size == capacity) {
            capacity = capacity ? capacity * 2 : 1;
            buf = static_cast<char *>(realloc(buf, capacity));
            if (!buf) {
                PANIC();
            }
        }

        size += fread(buf + size, 1, capacity - size, f);
        if (feof(f)) {
            break;
        } else if (ferror(f)) {
            PANIC();
        }
    }
    return {buf, size};
}

#endif
