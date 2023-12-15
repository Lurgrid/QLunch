//  Partie interface du module conf

#ifndef CONF__H
#define CONF__H

#include <stdlib.h>
#include <stdio.h>

#define SEPARATOR '='
#define COMMENT '#'

typedef struct keyval_t keyval_t;

extern keyval_t *conf_kv_init(const char *key, void *val, int (*hdl)(void *,
    const void *));

extern bool conf_kv_isclose(const keyval_t *kv);

extern int conf_process(keyval_t **akv, size_t len, FILE *f);

#endif
