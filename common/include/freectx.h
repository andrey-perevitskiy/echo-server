#ifndef FREECTX_H

enum type {
    IDFD = 0,
    IDMEM
};

struct free_context {
    enum type type;
    void * data;
    struct free_context * next;
};

struct free_context * freectx_alloc (void);
const struct free_context * freectx_append (struct free_context * fc, void * data, enum type type);
unsigned freectx_free (struct free_context * head);
void exit_free (struct free_context * head, const char * fmt, ...);

extern struct free_context * freectx;

#define FREECTX_H
#endif