#include "utilitaire.h"
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "conf.h"

//  CONF_ERROR : Macro fonction d'affichage d'erreur.
#define CONF_ERROR(akv, reason, ...) {                                         \
    fprintf(stderr, "Error on using the configuration file of the server. ");  \
    fprintf(stderr, (reason), __VA_ARGS__);                                    \
    array_map(akv, ARRAY_LENGTH(akv), sizeof(*akv),                            \
        (int (*)(void *))dispose_akv);                                         \
}

int array_map(void *base, size_t nmemb, size_t size, int (*f)(void *v)) {
  const char *endptr = (char *) base + nmemb * size;
  for (char *p = (char *) base; p < endptr; p += size) {
    if (f(p) != 0) {
      return -1;
    }
  }
  return 0;
}

int get_valid_name(char *n, const char *v, const char **err) {
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

int un_from_char(unsigned int *s, const char *v, const char **err) {
  char *end;
  errno = 0;
  long int r = strtol(v, &end, 10);
  if (*end != '\0') {
    *err = "Only number are accepted.";
    return -1;
  }
  if (errno == ERANGE || r > UINT_MAX) {
    *err = "The number is too big.";
    return -1;
  }
  if (r < 0) {
    *err = "Only positive number are accepted";
    return -1;
  }
  *s = (unsigned int) r;
  return 0;
}

int write_char_buff(int desc, const char *buffer, size_t length) {
  const char *pbuf = buffer;
  while (length > 0) {
    ssize_t n = write(desc, pbuf, length);
    if (n == -1) {
      return -1;
    }
    pbuf += n;
    length -= (size_t) n;
  }
  return 0;
}

//  keyval_t_is_null : Renvoie -1 si *t vaut NULL sinon 0.
static int keyval_t_is_null(keyval_t **t) {
  return *t == NULL ? -1 : 0;
}

//  dispose_akv : Libère *akv et renvoie 0.
static int dispose_akv(keyval_t **akv) {
  conf_kv_dispose(akv);
  return 0;
}

int process_conf_file(conf_t *conf) {
  FILE *f = fopen(CONFIG_NAME, "r");
  if (f == NULL) {
    fprintf(stderr, "Error on using the configuration file '%s'.", CONFIG_NAME);
    return -1;
  }
  keyval_t *akv[] = {
    conf_kv_init("FIFO_NAME", conf->fifo_name,
        (int (*)(void *, const char *, const char **))get_valid_name),
    conf_kv_init("FIFO_LENGTH", &conf->fifo_length,
        (int (*)(void *, const char *, const char **))un_from_char),
    conf_kv_init("FIFO_TIME", &conf->time,
        (int (*)(void *, const char *, const char **))un_from_char)
  };
  if (array_map(akv, ARRAY_LENGTH(akv), sizeof(*akv),
      (int (*)(void *))keyval_t_is_null) != 0) {
    fclose(f);
    CONF_ERROR(akv, ERR_MEMORY, "");
    return -1;
  }
  size_t index;
  const char *err;
  switch (conf_process(akv, ARRAY_LENGTH(akv), f, &index, &err)) {
    case ERROR_UNKNOWN:
      fclose(f);
      CONF_ERROR(akv, ERR_KEY, "");
      return -1;
    case ERROR_ALLOC:
      fclose(f);
      CONF_ERROR(akv, ERR_MEMORY, "");
      return -1;
    case ERROR_PROCESS:
      fclose(f);
      CONF_ERROR(akv, "The key %s, have the problem '%s'\n",
          conf_kv_getkey(akv[index]), err);
      return -1;
    default:
      break;
  }
  if (fclose(f) != 0) {
    CONF_ERROR(akv, ERR_FILE, "");
    return -1;
  }
  for (size_t i = 0; i < ARRAY_LENGTH(akv); ++i) {
    if (!conf_kv_isclose(akv[i])) {
      CONF_ERROR(akv, "the key %s is required.\n", conf_kv_getkey(akv[i]));
      return -1;
    }
  }
  array_map(akv, ARRAY_LENGTH(akv), sizeof(*akv),
      (int (*)(void *))dispose_akv);
  return 0;
}

int is_prefixe(const char *s1, const char *s2) {
  if (*s1 == '\0') {
    return *s2 == '\0' ? 0 : 1;
  }
  if (*s1 != *s2) {
    return -1;
  }
  return is_prefixe(s1 + 1, s2 + 1);
}
