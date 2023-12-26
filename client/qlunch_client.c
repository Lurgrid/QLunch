#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include "conf.h"
#include "shm_fifo.h"

#define CONFIG_NAME "../qlunch.conf"

static int kv_once_null(keyval_t **akv, size_t nmemb);

//  get_fifo_name : fonction testant si v est valide pour etre un nom de file
//    fifo du module shm_fifo. Si c'est le cas, n vaudra une copie de la chaine
//    v.
//    Renvoie 0 en cas de succé, sinon une valeur négatif et *err pointe sur une
//    chaine de caractère décrivant l'erreur.
static int get_fifo_name(char * n, const char * v, const char **err);

//  get_fifo_length : fonction testant si v est valide pour etre une taille de
//    file pour le module shm_fifo. Si c'est le cas, *s vaudra le nombre
//    représenter par v.
//    Renvoie 0 en cas de succé, sinon une valeur négatif et *err ponte sur une
//    chaine de aractère décrivant l'erreur.
static int get_fifo_length(unsigned int *s, const char *v, const char **err);

typedef struct {
  char fifo_name[NAME_MAX];
  size_t fifo_length;
} conf_t;

int main(void) {
  FILE *f = fopen(CONFIG_NAME, "r");
  if (f == NULL) {
    fprintf(stderr, "*** Error: Cannot found file \"%s\".\n", CONFIG_NAME);
    return EXIT_FAILURE;
  }
  conf_t conf;
  keyval_t *akv[] = {
    conf_kv_init("FIFO_NAME", conf.fifo_name,
        (int (*)(void *, const char *, const char **))get_fifo_name),
    conf_kv_init("FIFO_LENGTH", &conf.fifo_length,
        (int (*)(void *, const char *, const char **))get_fifo_length),
  };
  return EXIT_SUCCESS;
}

int get_fifo_name(char *n, const char *v, const char **err) {
  size_t i = 0;
  while (v[i] != '\0' && v[i] != '/' && i < NAME_MAX) {
    ++i;
  }
  if (i >= NAME_MAX) {
    *err = "Value is too long.";
    return -1;
  }
  if (v[i] == '/') {
    *err = "Contain the invalid '/' caracter.";
    return -1;
  }
  strcpy(n, v);
  return 0;
}

int get_fifo_length(unsigned int *s, const char *v, const char **err) {
  char *end;
  errno = 0;
  long int r = strtol(v, &end, 10);
  if (*end != '\0') {
    *err = "The length must be an integer.";
    return -1;
  }
  if (errno != ERANGE || r > UINT_MAX) {
    *err = "Too big number.";
    return -1;
  }
  if (r < 0) {
    *err = "Only positive number are excepted";
    return -1;
  }
  *s = (unsigned int) r;
  return 0;
}
