#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "conf.h"

#define ALEN(arr) (sizeof(arr) / sizeof *(arr))

#define NAME_MAX 255

typedef struct {
  char fifo_name[NAME_MAX + 1];
  size_t fifo_length;
} conf_t;

static int str_cpy(char *d, const char *s);
static int str_tos(size_t *d, const char *s);
static int kv_all_close(keyval_t **akv, size_t nmemb);
static int kv_once_null(keyval_t **akv, size_t nmemb);

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "*** Syntax: %s FILE\n", argv[0]);
    return EXIT_FAILURE;
  }
  FILE *f = fopen(argv[1], "r");
  if (f == NULL) {
    fprintf(stderr, "*** Error: Cannot found file \"%s\".\n", argv[1]);
    return EXIT_FAILURE;
  }
  conf_t conf;
  keyval_t *akv[] = {
    conf_kv_init("FIFO_NAME", conf.fifo_name,
        (int (*)(void *, const void *))str_cpy),
    conf_kv_init("FIFO_LENGTH", &conf.fifo_length,
        (int (*)(void *, const void *))str_tos),
  };
  if (kv_once_null(akv, ALEN(akv)) != 0) {
    fprintf(stderr, "*** Error: Not enought memory\n");
    return EXIT_FAILURE;
  }
  if (conf_process(akv, ALEN(akv), f) != 0) {
    fprintf(stderr, "*** Error: Cannot process the config file\n");
    return EXIT_FAILURE;
  }
  if (kv_all_close(akv, ALEN(akv)) != 0) {
    fprintf(stderr, "*** Error: All key is needed\n");
    return EXIT_FAILURE;
  }
  printf("fifo_name: %s\n", conf.fifo_name);
  printf("fifo_length: %zu\n", conf.fifo_length);
  return EXIT_SUCCESS;
}

int kv_once_null(keyval_t **akv, size_t nmemb) {
  for (size_t k = 0; k < nmemb; ++k) {
    if (akv[k] == NULL) {
      return -1;
    }
  }
  return 0;
}

int kv_all_close(keyval_t **akv, size_t nmemb) {
  for (size_t k = 0; k < nmemb; ++k) {
    if (!conf_kv_isclose(akv[k])) {
      return -1;
    }
  }
  return 0;
}

int str_cpy(char *d, const char *s) {
  if (strlen(s) >= NAME_MAX) {
    return -1;
  }
  strcpy(d, s);
  return 0;
}

int str_tos(size_t *d, const char *s) {
  char *end;
  errno = 0;
  long int r = strtol(s, &end, 10);
  if (errno != 0 || *end != '\0') {
    return -1;
  }
  if (r < 0) {
    return -1;
  }
  *d = (size_t) r;
  return 0;
}
