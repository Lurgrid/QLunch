#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include "conf.h"
#include "shm_fifo.h"
#include "da.h"

//  CONFIG_NAME : Nom du fichier de configuration.
#define CONFIG_NAME "../qlunch.conf"

//  ARRAY_LENGTH(arr) : Renvoie la longeure du tableau arr
#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof *(arr))

//  BUFF_SIZE : Taille du buffer de lecture sur l'entrée standard
#define BUFF_LENGTH 4096

// NUMBER_PIPE : nombre de tube
#define NUMBER_PIPE 3

//  NUMBER_THREAD : nombre de thread
#define NUMBER_THREAD 2

typedef struct {
  char fifo_name[NAME_MAX];
  unsigned int fifo_length;
} conf_t;

typedef struct {
  int in;
  int out;
} file_t;

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

static void dispose_all(fifo_t **fifo, char tubes[NUMBER_PIPE][PATH_MAX]);

static int write_char(int desc, da *buff);

static void *read_write(file_t *f);

int main(void) {
  conf_t conf;
  process_conf_file(&conf);
  char tubes[NUMBER_PIPE][PATH_MAX];
  for (size_t i = 0; i < NUMBER_PIPE; ++i) {
    if (snprintf(tubes[i], PATH_MAX - 1, "%d_%zu", getpid(), i) <= 0) {
      fprintf(stderr, "Error on creating communication tool.\n");
      return EXIT_FAILURE;
    }
  }
  if (mkfifo(tubes[0], S_IRUSR | S_IWUSR) != 0) {
    fprintf(stderr, "Error on creating communication tool.\n");
    return EXIT_FAILURE;
  }
  if (mkfifo(tubes[1], S_IRUSR | S_IWUSR) != 0) {
    fprintf(stderr, "Error on creating communication tool.\n");
    unlink(tubes[0]);
    return EXIT_FAILURE;
  }
  if (mkfifo(tubes[2], S_IRUSR | S_IWUSR) != 0) {
    fprintf(stderr, "Error on creating communication tool.\n");
    unlink(tubes[0]);
    unlink(tubes[1]);
    return EXIT_FAILURE;
  }
  fifo_t *f = shm_fifo_open(conf.fifo_name, conf.fifo_length);
  if (f == NULL) {
    dispose_all(&f, tubes);
    fprintf(stderr,
        "Error on creating communication tool. Not enougth memory.\n");
    return EXIT_FAILURE;
  }
  char buff[BUFF_LENGTH];
  da *line = da_empty(sizeof(char));
  if (line == NULL) {
    dispose_all(&f, tubes);
    fprintf(stderr,
        "Error on creating communication tool. Not enougth memory.\n");
    return EXIT_FAILURE;
  }
  int tubes_desc[NUMBER_PIPE] = {
    -1
  };
  ssize_t n;
  pthread_t threads[NUMBER_THREAD];
  file_t *files = malloc(sizeof *files * NUMBER_THREAD);
  if (files == NULL) {
    dispose_all(&f, tubes);
    fprintf(stderr,
        "Error on communication the command to the server. Not enougth memory."
        "\n");
    exit(EXIT_FAILURE);
  }
  files[0].in = tubes_desc[1];
  files[0].out = STDOUT_FILENO;
  files[1].in = tubes_desc[2];
  files[1].out = STDERR_FILENO;
  do {
    n = read(STDIN_FILENO, buff, BUFF_LENGTH);
    if (n == -1) {
      dispose_all(&f, tubes);
      fprintf(stderr, "Error on reading command.\n");
      return EXIT_FAILURE;
    }
    for (size_t i = 0; i < (size_t) n; ++i) {
      if (*(buff + i) != '\n') {
        if (da_add(line, buff + i) == NULL) {
          dispose_all(&f, tubes);
          fprintf(stderr, "Error on reading command. Not enougth memory.\n");
          return EXIT_FAILURE;
        }
        continue;
      }
      if (shm_fifo_enqueue(f, getpid()) != 0) {
        dispose_all(&f, tubes);
        fprintf(stderr, "Error on communication the command to the server.\n");
        return EXIT_FAILURE;
      }
      if (tubes_desc[0] == -1) {
        for (size_t i = 0; i < NUMBER_PIPE; ++i) {
          tubes_desc[i] = open(tubes[i], (i != 0 ? O_RDONLY : O_WRONLY));
          if (tubes_desc[i] == -1) {
            dispose_all(&f, tubes);
            fprintf(stderr,
                "Error on communication the command to the server.\n");
            exit(EXIT_FAILURE);
          }
        }
        for (size_t i = 0; i < NUMBER_THREAD; ++i) {
          if (pthread_create(threads + i, NULL, (void *(*)(void *))read_write,
              files + i) != 0) {
            free(files);
            dispose_all(&f, tubes);
            fprintf(stderr,
                "Error on communication the command to the server. Not enougth "
                "memory.\n");
            exit(EXIT_FAILURE);
          }
        }
      }
      if (write_char(tubes_desc[0], line) != 0) {
        dispose_all(&f, tubes);
        fprintf(stderr, "Error on communication the command to the server.\n");
        exit(EXIT_FAILURE);
      }
      for (size_t i = 0; i < NUMBER_THREAD; ++i) {
        file_t *v;
        if (pthread_join(threads[i], (void *) &v) != 0 || v == NULL) {
          dispose_all(&f, tubes);
          fprintf(stderr,
              "Error on communication the command to the server.\n");
          exit(EXIT_FAILURE);
        }
      }
      da_reset(line);
    }
  } while (n > 0);
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

void dispose_all(fifo_t **fifo, char tubes[NUMBER_PIPE][PATH_MAX]) {
  shm_fifo_dispose(fifo);
  for (size_t i = 0; i < NUMBER_PIPE; ++i) {
    unlink(tubes[i]);
  }
}

int write_char(int desc, da *buff) {
  size_t nb_write = 0;
  while (nb_write != da_length(buff) - 1) {
    ssize_t n = write(desc, da_nth(buff, nb_write),
        da_length(buff) - nb_write - 1);
    if (n == -1) {
      return -1;
    }
    nb_write += (size_t) n;
  }
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

void *read_write(file_t *f) {
  char buffer[BUFF_LENGTH];
  ssize_t n;
  do {
    n = read(f->in, buffer, BUFF_LENGTH);
    if (n == -1) {
      return NULL;
    }
    write_char_buff(f->out, buffer, (size_t) n);
  } while (n > 0);
  return f;
}
