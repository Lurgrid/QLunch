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

static int str_cpy(char *d, const char *s, const char **err);
static int str_tos(size_t *d, const char *s, const char **err);
static int kv_all_close(keyval_t **akv, size_t nmemb);
static int kv_once_null(keyval_t **akv, size_t nmemb);
static void kv_all_dispose(keyval_t **akv, size_t nmemb);

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
  int r = EXIT_FAILURE;
  conf_t conf;
  keyval_t *akv[] = {
    conf_kv_init("FIFO_NAME", conf.fifo_name,
        (int (*)(void *, const char *, const char **))str_cpy),
    conf_kv_init("FIFO_LENGTH", &conf.fifo_length,
        (int (*)(void *, const char *, const char **))str_tos),
  };
  if (kv_once_null(akv, ALEN(akv)) != 0) {
    goto err_allocation;
  }
  const char *err;
  int cp;
  size_t index;
  if ((cp = conf_process(akv, ALEN(akv), f, &index, &err)) != DONE) {
    switch (cp) {
      case ERROR_UNKNOWN:
        fprintf(stderr, "*** Error: The config file, contains a invalid key\n");
        break;
      case ERROR_ALLOC:
        goto err_allocation;
      default:
        fprintf(stderr, "*** Error: \"%s\", on key \"%s\".\n", err,
            conf_kv_getkey(akv[index]));
    }
    goto error;
  }
  if (kv_all_close(akv, ALEN(akv)) != 0) {
    fprintf(stderr, "*** Error: All key is needed\n");
    goto error;
  }
  printf("fifo_name: %s\n", conf.fifo_name);
  printf("fifo_length: %zu\n", conf.fifo_length);
  if (fclose(f) != 0) {
    f = NULL;
    goto err_file;
  }
  f = NULL;
  goto dispose; // imo avec le prof je suis pas sur que mettre des goto est une bonne idées
err_file:
  fprintf(stderr, "*** Error: On processing the file\n");
  goto error;
err_allocation:
  fprintf(stderr, "*** Error: Not enought memory\n");
error:
  r = EXIT_FAILURE;
dispose:
  kv_all_dispose(akv, ALEN(akv));
  if (f != NULL) {
    fclose(f);
  }
  return r;
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

void kv_all_dispose(keyval_t **akv, size_t nmemb) {
  for (size_t k = 0; k < nmemb; ++k) {
    conf_kv_dispose(&akv[k]);
  }
}

int str_cpy(char *d, const char *s, const char **err) {
  if (strlen(s) >= NAME_MAX) {
    *err = "Value is too long";
    return -1;
  }
  strcpy(d, s);
  return 0;
}

int str_tos(size_t *d, const char *s, const char **err) {
  char *end;
  errno = 0;
  long int r = strtol(s, &end, 10);
  if (errno != 0 || *end != '\0') {// il faut différencier le cas de c'est pas un nombre de le nombre est trop grand
    *err = "Invalid number";
    return -1;
  }
  if (r < 0) {
    *err = "Only positive number";
    return -1;
  }
  *d = (size_t) r;
  return 0;
}
