#ifndef _USERLOADER_H_
#define _USERLOADER_H_

#include "config.h"

#define DLFUNCS_TYPE_ON_UP      1
#define DLFUNCS_TYPE_ON_DOWN    2
#define DLFUNCS_TYPE_POST_UP    3
#define DLFUNCS_TYPE_POST_DOWN  4

#define DLFUNCS_GENRE_DL        1
#define DLFUNCS_GENRE_PERL      2

struct dlfuncs {
    char genre;
    char *path;
    char *func;
    void *handle;
    char type;
    int (*handler)(struct interface, struct interface *, struct interface);
};

int register_shared(char *path, char *func, char type);
int register_perl(char *function, char type);
int deregister_all_shared();
int execute_all_user(struct interface pseudo,
                     struct interface *extras,
                     struct interface real,
                     char type);
int execute_all_user_simple(char type);

#endif
