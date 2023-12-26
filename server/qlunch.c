#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include "shm_fifo.h"
#include "conf.h"

//  CONFIG_NAME : Nom du fichier de configuration.
#define CONFIG_NAME "../qlunch.conf"

//  ARRAY_LENGTH(arr) : Renvoie la longeure du tableau arr
#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof *(arr))

//  BUFF_SIZE : Taille du buffer de lecture sur l'entrée standard
#define BUFF_LENGTH 4096

typedef struct {
  char fifo_name[NAME_MAX];
  unsigned int fifo_length;
} conf_t;

//  kv_once_null : Renvoie 0 si aucun élément akv de longeur nmemb est NULL,
//    sinon revnoie -1.
static int kv_once_null(keyval_t **akv, size_t nmemb);

//  get_fifo_name : fonction testant si v est valide pour etre un nom de file
//    fifo du module shm_fifo. Si c'est le cas, n vaudra une copie de la chaine
//    v.
//    Renvoie 0 en cas de succé, sinon une valeur négatif et *err pointe sur une
//    chaine de caractère décrivant l'erreur.
static int get_fifo_name(char *n, const char *v, const char **err);

//  get_fifo_length : fonction testant si v est valide pour etre une taille de
//    file pour le module shm_fifo. Si c'est le cas, *s vaudra le nombre
//    représenter par v.
//    Renvoie 0 en cas de succé, sinon une valeur négatif et *err ponte sur une
//    chaine de aractère décrivant l'erreur.
static int get_fifo_length(unsigned int *s, const char *v, const char **err);

static void process_conf_file(conf_t *conf);

static void dispose_akv(keyval_t **akv, size_t nmemb);

int main(void) {
  conf_t conf;
  process_conf_file(&conf);
  fifo_t *f = shm_fifo_init(conf.fifo_name, conf.fifo_length);
  if (f == NULL) {
    fprintf(stderr, "Error not enougth memory.\n");
    return EXIT_FAILURE;
  }
  switch (fork()) {
    case -1:
      fprintf(stderr, "Error on intialising the server.\n");
      return EXIT_FAILURE;
    case 0:
      if (setsid() == (pid_t) -1) {
        fprintf(stderr, "Error on intialising the server.\n");
        return EXIT_FAILURE;
      }
      //if (close(STDIN_FILENO) != 0 ||  close(STDOUT_FILENO) != 0
          //||  close(STDERR_FILENO) != 0) {
        //return EXIT_FAILURE;
      //}
      pid_t p;
      while ((p = shm_fifo_dequeue(f)) != (pid_t) -1) {

      }
    default:
      break;
  }
  return EXIT_SUCCESS;
}

int kv_once_null(keyval_t **akv, size_t nmemb) {
  for (size_t i = 0; i < nmemb; ++i) {
    if (akv[i] == NULL) {
      return -1;
    }
  }
  return 0;
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
  if (errno == ERANGE || r > UINT_MAX) {
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

void process_conf_file(conf_t *conf) {
  FILE *f = fopen(CONFIG_NAME, "r"); // a voir
  if (f == NULL) {
    fprintf(stderr, "*** Error: Cannot found file \"%s\".\n", CONFIG_NAME);
    exit(EXIT_FAILURE);
  }
  keyval_t *akv[] = {
    conf_kv_init("FIFO_NAME", conf->fifo_name,
        (int (*)(void *, const char *, const char **))get_fifo_name),
    conf_kv_init("FIFO_LENGTH", &conf->fifo_length,
        (int (*)(void *, const char *, const char **))get_fifo_length),
  };
  if (kv_once_null(akv, ARRAY_LENGTH(akv)) != 0) {
    fclose(f);
    fprintf(stderr, "Error on initlisasing the client. Not enougth memory.\n");
    dispose_akv(akv, ARRAY_LENGTH(akv));
    exit(EXIT_FAILURE);
  }
  int r;
  size_t index;
  const char *err;
  switch ((r = conf_process(akv, ARRAY_LENGTH(akv), f, &index, &err))) {
    case ERROR_UNKNOWN:
      fprintf(stderr, "Error the config file, contains a invalid key\n");
      break;
    case ERROR_ALLOC:
      fprintf(stderr, "Error not enougth memory\n");
      break;
    case ERROR_PROCESS:
      fprintf(stderr, "Error on the key %s, '%s'\n", conf_kv_getkey(akv[index]),
          err);
      break;
    default:
      break;
  }
  if (fclose(f) != 0) {
    fprintf(stderr, "Error on closing the conf file.\n");
    dispose_akv(akv, ARRAY_LENGTH(akv));
    exit(EXIT_FAILURE);
  }
  if (r != DONE) {
    dispose_akv(akv, ARRAY_LENGTH(akv));
    exit(EXIT_FAILURE);
  }
  for (size_t i = 0; i < ARRAY_LENGTH(akv); ++i) {
    if (!conf_kv_isclose(akv[i])) {
      fprintf(stderr, "Error the key %s is required.\n",
          conf_kv_getkey(akv[i]));
      dispose_akv(akv, ARRAY_LENGTH(akv));
      exit(EXIT_FAILURE);
    }
  }
  dispose_akv(akv, ARRAY_LENGTH(akv));
}

void dispose_akv(keyval_t **akv, size_t nmemb) {
  for (size_t i = 0; i < nmemb; ++i) {
    conf_kv_dispose(&akv[i]);
  }
}
