// conf.c : partie implantation de l'interface conf.h

#include "conf.h"
#include "da.h"
#include <stdio.h>
#include <string.h>

struct keyval_t {
  const char *key;
  void *val;
  int (*hdl)(void *, const char *, const char **err);
  bool close;
};

keyval_t *conf_kv_init(const char *key, void *val, int (*hdl)(void *,
    const char *, const char **err)) {
  keyval_t *kv = malloc(sizeof *kv);
  if (kv == NULL) {
    return NULL;
  }
  kv->key = key;
  kv->val = val;
  kv->hdl = hdl;
  kv->close = 0;
  return kv;
}

bool conf_kv_isclose(const keyval_t *kv) {
  return kv->close;
}

const char *conf_kv_getkey(const keyval_t *kv) {
  return kv->key;
}

void conf_kv_dispose(keyval_t **kv) {
  if (kv != NULL) {
    free(*kv);
    *kv = NULL;
  }
}

//  fnlines : lis sur le flot associé à f, tous les charactères d'une ligne.
//    Tente de les ajouté à t. Si a la fin de la lecture, aucune erreur n'a
//    été détécter, tente d'ajouté '\0' à t.
//    Retourne une valeur nulle en cas de succée, sinon une valeur strictement
//    positif en cas d'erreur sur la lecture du fichier. Enfin une valeur
//    strictement négatif est renvoyé en cas d'erreur d'allocation.
static int fnlines(FILE * restrict f, da * restrict t) {
  int c;
  while ((c = fgetc(f)) != EOF && c != '\n') {
    if (da_add(t, &c) == NULL) {
      return -1;
    }
  }
  if (ferror(f) != 0) {
    return 1;
  }
  c = '\0';
  if (da_add(t, &c) == NULL) {
    return -1;
  }
  return 0;
}

//  prefix : Renvoie NULL si s1 n'est pas prefix de s2, si s1 est egale a s2
//    alors renvois le pointeur sur le carractère nul de s2 sinon renvois le
//    pointeur sur le premier carractère de s2 qui n'appartient pas a s1.
static const char *prefix(const char *s1, const char *s2) {
  while (1) {
    if (*s1 == '\0') {
      return s2;
    }
    if (*s2 == '\0' || *s1 != *s2) {
      return NULL;
    }
    ++s1;
    ++s2;
  }
}

int conf_process(keyval_t **akv, size_t len, FILE *f, const char **err) {
  da *line = da_empty(sizeof(char));
  if (line == NULL) {
    return ERROR_ALLOC;
  }
  while (!feof(f) && fnlines(f, line) == 0) {
    if (da_length(line) > 1 && *((char *) da_nth(line, 0)) != COMMENT) {
      const char *l = (const char *) da_nth(line, 0);
      keyval_t **cur = akv;
      while (cur < akv + len) {
        const char *t = prefix((*cur)->key, l);
        if (t != NULL && *t == SEPARATOR) {
          if ((*cur)->hdl((*cur)->val, (void *) (t + 1), err) != 0) {
            da_dispose(&line);
            return (int) (cur - akv) / (int) sizeof *akv + 1;
          }
          (*cur)->close = true;
          break;
        }
        ++cur;
      }
      if (cur == akv + len) {
        da_dispose(&line);
        return ERROR_UNKNOWN;
      }
    }
    da_reset(line);
  }
  da_dispose(&line);
  return 0;
}
