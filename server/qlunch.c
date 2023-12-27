#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif
#define _XOPEN_SOURCE 500

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdint.h>
#include <wait.h>
#include <signal.h>
#include "shm_fifo.h"
#include "conf.h"
#include "da.h"
#include "analyse.h"

//  CONFIG_NAME : Nom du fichier de configuration.
#define CONFIG_NAME "../qlunch.conf"

//  ARRAY_LENGTH(arr) : Renvoie la longeure du tableau arr
#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof *(arr))

//  BUFF_SIZE : Taille du buffer de lecture sur l'entrée standard
#define BUFF_LENGTH 4096

// NUMBER_PIPE : nombre de tube
#define NUMBER_PIPE 3

#define PIPE_TOKEN '|'

#define TRACK fprintf(stderr, "%s: %d\n", __func__, __LINE__)

typedef struct {
  char fifo_name[NAME_MAX];
  unsigned int fifo_length;
  unsigned int time;
} conf_t;

typedef struct {
  conf_t *conf;
  pid_t pid;
} cmd_t;

static conf_t *conf;

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

static void *cmd_process(cmd_t *c);

static int pipe_engine(char ***cmds, size_t nmemb, int *tubes_desc);

static void gestionnaire(int signum);

int main(void) {
  conf = malloc(sizeof *conf);
  if (conf == NULL) {
    fprintf(stderr, "Error not enougth memory.\n");
    return EXIT_FAILURE;
  }
  process_conf_file(conf);
  switch (fork()) {
    case -1:
      fprintf(stderr, "Error on intialising the server.\n");
      shm_fifo_unlink(conf->fifo_name);
      free(conf);
      return EXIT_FAILURE;
    case 0:
      if (setsid() == (pid_t) -1) {
        free(conf);
        fprintf(stderr, "Error on intialising the server.\n");
        return EXIT_FAILURE;
      }
      if (close(STDIN_FILENO) != 0 || close(STDOUT_FILENO) != 0
          || close(STDERR_FILENO) != 0) {
        free(conf);
        return EXIT_FAILURE;
      }
      fifo_t *f = shm_fifo_init(conf->fifo_name, conf->fifo_length);
      if (f == NULL) {
        fprintf(stderr, "Error not enougth memory.\n");
        free(conf);
        return EXIT_FAILURE;
      }
      struct sigaction act = {
        .sa_handler = gestionnaire,
        .sa_flags = SA_NODEFER,
      };
      if (sigemptyset(&act.sa_mask) != 0) {
        shm_fifo_unlink(conf->fifo_name);
        fprintf(stderr, "Error not enougth memory.\n");
        free(conf);
        return EXIT_FAILURE;
      }
      if (sigaction(SIGTERM, &act, NULL) != 0) {
        shm_fifo_unlink(conf->fifo_name);
        fprintf(stderr, "Error not enougth memory.\n");
        free(conf);
        return EXIT_FAILURE;
      }
      if (sigaction(SIGHUP, &act, NULL) != 0) {
        shm_fifo_unlink(conf->fifo_name);
        fprintf(stderr, "Error not enougth memory.\n");
        free(conf);
        return EXIT_FAILURE;
      }
      pid_t p;
      pthread_t t;
      while (1) {
        if ((p = shm_fifo_dequeue(f)) != (pid_t) -1) {
          cmd_t *c = malloc(sizeof *c);
          if (c == NULL) {
            continue;
          }
          c->conf = conf;
          c->pid = p;
          if (pthread_create(&t, NULL, (void *(*)(void *))cmd_process,
              c) != 0) {
            shm_fifo_unlink(conf->fifo_name);
            free(conf);
            free(c);
            return EXIT_FAILURE;
          }
          sleep(conf->time);
        }
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
    conf_kv_init("FIFO_TIME", &conf->time,
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

static int write_char_buff(int desc, const char *buffer, size_t length) {
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

void *cmd_process(cmd_t *c) {
  da *cmd = da_empty(sizeof(char));
  if (cmd == NULL) {
    free(c);
    return NULL;
  }
  char tubes[NUMBER_PIPE][PATH_MAX];
  for (size_t i = 0; i < NUMBER_PIPE; ++i) {
    if (snprintf(tubes[i], PATH_MAX - 1, "/tmp/%d_%zu", c->pid, i) <= 0) {
      free(c);
      da_dispose(&cmd);
      return NULL;
    }
  }
  int tubes_desc[NUMBER_PIPE];
  for (size_t i = 0; i < NUMBER_PIPE; ++i) {
    tubes_desc[i] = open(tubes[i], (i != 0 ? O_WRONLY : O_RDONLY));
    if (tubes_desc[i] == -1) {
      for (size_t j = 0; j < i; ++j) {
        close(tubes_desc[j]);
      }
      free(c);
      da_dispose(&cmd);
      return NULL;
    }
  }
  char buff[BUFF_LENGTH];
  ssize_t n;
  do {
    n = read(tubes_desc[0], buff, BUFF_LENGTH);
    if (n == -1) {
      free(c);
      da_dispose(&cmd);
      return NULL;
    }
    for (size_t i = 0; i < (size_t) n; ++i) {
      if (da_add(cmd, buff + i) == NULL) {
        const char *err = "Not enought memory.\n";
        write_char_buff(tubes_desc[2], err, strlen(err));
        for (size_t j = 0; j < NUMBER_PIPE; ++j) {
          close(tubes_desc[j]);
        }
        da_dispose(&cmd);
        free(c);
        return NULL;
      }
      if (buff[i] == '\0') {
        n = 0;
        break;
      }
    }
  } while (n > 0);
  size_t nb_p = 0;
  size_t nb_c = 0;
  size_t cur = 0;
  char *mcmd = da_nth(cmd, 0);
  while (mcmd[cur] != '\0') {
    if (mcmd[cur] == PIPE_TOKEN) {
      if (nb_c == 0) {
        const char *err = "Invalid command send.\n";
        write_char_buff(tubes_desc[2], err, strlen(err));
        for (size_t j = 0; j < NUMBER_PIPE; ++j) {
          close(tubes_desc[j]);
        }
        da_dispose(&cmd);
        free(c);
        return NULL;
      }
      mcmd[cur] = '\0';
      ++nb_p;
    } else if (mcmd[cur] != ' ' && (nb_c == 0 || nb_p == nb_c)) {
      ++nb_c;
    }
    ++cur;
  }
  if (nb_p >= nb_c) {
    const char *err = "Invalid command send.\n";
    write_char_buff(tubes_desc[2], err, strlen(err));
    for (size_t j = 0; j < NUMBER_PIPE; ++j) {
      close(tubes_desc[j]);
    }
    da_dispose(&cmd);
    free(c);
    return NULL;
  }
  if (nb_c > SIZE_MAX / sizeof(char **)) {
    const char *err = "Not enought memory.\n";
    write_char_buff(tubes_desc[2], err, strlen(err));
    for (size_t j = 0; j < NUMBER_PIPE; ++j) {
      close(tubes_desc[j]);
    }
    da_dispose(&cmd);
    free(c);
    return NULL;
  }
  char ***cmds = malloc(sizeof(char **) * nb_c);
  if (cmds == NULL) {
    const char *err = "Not enought memory.\n";
    write_char_buff(tubes_desc[2], err, strlen(err));
    for (size_t j = 0; j < NUMBER_PIPE; ++j) {
      close(tubes_desc[j]);
    }
    da_dispose(&cmd);
    free(c);
    return NULL;
  }
  cur = 0;
  for (size_t i = 0; i < nb_c; ++i) {
    cmds[i] = analyse_arg(mcmd + cur);
    cur += strlen(mcmd + cur) + 1;
  }
  da_dispose(&cmd);
  pipe_engine(cmds, nb_c, tubes_desc);
  for (size_t j = 0; j < NUMBER_PIPE; ++j) {
    close(tubes_desc[j]);
  }
  for (size_t i = 0; i < nb_c; ++i) {
    dispose_arg(cmds[i]);
  }
  free(c);
  return NULL;
}

int pipe_engine(char ***cmds, size_t nmemb, int *tubes_desc) {
  if (nmemb == 0) {
    return -1;
  }
  int tub[2];
  int cin = -1;
  for (size_t i = 0; i < nmemb; ++i) {
    if (i == nmemb - 1) {
      pid_t p;
      switch ((p = fork())) {
        case -1:
          return -1;
        case 0:
          if (cin != -1 && dup2(cin, STDIN_FILENO) == -1) {
            exit(EXIT_FAILURE);
          }
          if (dup2(tubes_desc[1], STDOUT_FILENO) == -1) {
            exit(EXIT_FAILURE);
          }
          if (dup2(tubes_desc[2], STDERR_FILENO) == -1) {
            exit(EXIT_FAILURE);
          }
          execvp(cmds[i][0], cmds[i]);
          const char *err = "Unknown command.\n";
          write_char_buff(tubes_desc[2], err, strlen(err));
          exit(EXIT_FAILURE);
        default:
          if (wait(NULL) == (pid_t) -1) {
            return -1;
          }
          return 0;
      }
    }
    if (pipe(tub) != 0) {
      return -1;
    }
    switch (fork()) {
      case -1:
        return -1;
      case 0:
        if (close(tub[0]) != 0) {
          exit(EXIT_FAILURE);
        }
        if (cin != -1 && dup2(cin, STDIN_FILENO) == -1) {
          exit(EXIT_FAILURE);
        }
        if (cin != -1 && close(cin) != 0) {
          exit(EXIT_FAILURE);
        }
        if (dup2(tub[1], STDOUT_FILENO) == -1) {
          exit(EXIT_FAILURE);
        }
        if (close(tub[1]) != 0) {
          exit(EXIT_FAILURE);
        }
        if (dup2(tubes_desc[2], STDERR_FILENO) == -1) {
          exit(EXIT_FAILURE);
        }
        execvp(cmds[i][0], cmds[i]);
        const char *err = "Unknown command.\n";
        write_char_buff(tubes_desc[2], err, strlen(err));
        exit(EXIT_FAILURE);
      default:
        if (close(tub[1]) != 0) {
          return -1;
        }
        if (cin != -1 && close(cin) != 0) {
          return -1;
        }
        cin = tub[0];
    }
  }
  return 0;
}

void gestionnaire(int signum) {
  switch (signum) {
    case SIGTERM:
      shm_fifo_unlink(conf->fifo_name);
      kill(0, SIGKILL);
      exit(EXIT_SUCCESS);
      break;
    case SIGHUP:
      process_conf_file(conf);
      return;
    default:
      return;
  }
}
