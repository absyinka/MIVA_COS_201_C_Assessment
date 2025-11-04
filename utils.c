#include "student.h"

/* ---------- Memory Management ---------- */

char *safe_strdup(const char *s) {
    if (!s) {
        return NULL;
    }

    size_t len = strlen(s) + 1;
    char *copy = malloc(len);

    if (copy) {
        memcpy(copy, s, len);
    }

    return copy;
}

char *read_line(const char *prompt) {
    if (prompt) {
        printf("%s", prompt);
        fflush(stdout);
    }
    
    size_t capacity = 128;
    size_t len = 0;
    char *buf = malloc(capacity);

    if (!buf) {
        return NULL;
    }

    int c;
    while ((c = getchar()) != EOF && c != '\n') {
        if (len + 1 >= capacity) {
            capacity *= 2;
            char *tmp = realloc(buf, capacity);
            if (!tmp) {
                free(buf);
                return NULL;
            }
            buf = tmp;
        }
        buf[len++] = (char)c;
    }
    buf[len] = '\0';
    return buf;
}

void trim_inplace(char *s) {
    if (!s) {
        return;
    }

    // Left trim
    char *start = s;

    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    if (start != s) {
        memmove(s, start, strlen(start) + 1);
    }

    // Right trim
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n-1])) {
        s[--n] = '\0';
    }
}

