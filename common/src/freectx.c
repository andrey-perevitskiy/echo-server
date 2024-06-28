#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "freectx.h"

struct free_context *
freectx_alloc (void)
{
    return calloc(1, sizeof(struct free_context));
}

const struct free_context *
freectx_append (struct free_context * fc, void * data, enum type type)
{
    if (fc->data == NULL) {
        fc->type = type;
        fc->data = data;
        return fc;
    }

    // O(n)
    while (fc->next != NULL)
        fc = fc->next;
    fc->next = freectx_alloc();
    fc->next->type = type;
    fc->next->data = data;
    return fc->next;
}

unsigned
freectx_free (struct free_context * head)
{
    struct free_context * node;
    unsigned q = 0;

    node = head;
    while (head->next != NULL) {
        node = head->next;
        free(head);
        head = node;
        q++;
    }
    free(head);

    return q + 1;
}

void
exit_free (struct free_context * head, const char * fmt, ...)
{
    va_list ap;
    struct free_context * node;
    int * fd;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    for (node = head; node != NULL; node = node->next) {
        if (node->data == NULL)
            continue;
        switch (node->type) {
        case IDFD:
            fd = node->data;
            if (*fd != -1) {
                close(*fd);
                *fd = -1;
            }
            break;
        case IDMEM:
            free(node->data);
            node->data = NULL;
            break;
        default:
            break;
        }
    }
    freectx_free(head);
    exit(EXIT_FAILURE);
}