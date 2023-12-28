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
#include <pthread.h>
#include <fcntl.h>
#include <stdint.h>
#include <wait.h>
#include <signal.h>
#include "shm_fifo.h"
#include "conf.h"
#include "da.h"
#include "analyse.h"
#include "utilitaire.h"

//  BUFF_SIZE : Taille des divers buffer de lecture et d'ecriture sur les
//    différent pipes utliser.
#define BUFF_LENGTH 4096

// NUMBER_PIPE : nombre de tube utiliser par le programme.
#define NUMBER_PIPE 3

// PIPE_TOKEN : caractère représentant un pipe.
#define PIPE_TOKEN '|'

// INIT_SERV_ERROR : Macro fonction gérant les erreurs d'initialisation du 
//    server.
#define INIT_SERV_ERROR(conf, reason, ...) {                                   \
    fprintf(stderr, "Error on lunching the server. ");                         \
    fprintf(stderr, (reason), __VA_ARGS__);                                    \
    free((conf));                                                              \
}

// SERV_ERROR : Macro fonction gérant les erreurs de gestion du serveur.
#define SERV_ERROR(conf, shm) {                                                \
    shm_fifo_dispose(&(shm));                                                  \
    shm_fifo_unlink(conf->fifo_name);                                          \
    free((conf));                                                              \
}

//  CMD_ERROR : Macro fontion gérant les erreurs de comamndes.
#define CMD_ERROR(c, tubes, length, reason) {                                  \
    write_char_buff((tubes_desc)[2], (reason), strlen(reason));                \
    for (size_t j = 0; j < (length); ++j) {                                    \
      close((tubes)[j]);                                                       \
    }                                                                          \
    da_dispose((c));                                                           \
}

//  conf : Variable static décrivant la configuration du server. Cette variable
//    est liée au contenu du fichier de chemin d'accés CONFIG_NAME. 
static conf_t *conf;

// sig_handler : Gestionnaire de signaux du server.
static void sig_handler(int signum);

//  cmd_process : Fonction gérant la gestion des commandes recu de 
//    l'utilisateur. 
//    Renvoie NULL.
static void *cmd_process(pid_t *p);

//  pipe_engine : Fonction gérant les commandes de la forme : 
//    cmd | cmd2 | ... | cmdn, recu par le client.
//    Renvoie une valeur nulle en cas de réussite. Une valeur différente de 0 
//    dans le cas contraire.
static int pipe_engine(char ***cmds, size_t nmemb, int *tubes_desc);

int main(void) {
  conf = malloc(sizeof *conf);
  if (conf == NULL) {
    fprintf(stderr, "Error not enougth memory.\n");
    return EXIT_FAILURE;
  }
  if (process_conf_file(conf) != 0) {
    free(conf);
    return EXIT_FAILURE;
  }
  switch (fork()) {
    case -1:
      INIT_SERV_ERROR(conf, "Creating processes wasn't succesfull. \n%s", "");
      return EXIT_FAILURE;
    case 0:
      // mis en place les actions du server en fonction de signaux recu
      struct sigaction act = {
        .sa_handler = sig_handler,
        .sa_flags = SA_NODEFER,
      };
      if (sigemptyset(&act.sa_mask) != 0
          || sigaction(SIGTERM, &act, NULL) != 0
          || sigaction(SIGHUP, &act, NULL) != 0) {
        INIT_SERV_ERROR(conf, "Wasn't able asing action to signal. \n%s", "");
        return EXIT_FAILURE;
      }
      // fermeture des sorties/entréer standart et changement de groupe
      if (setsid() == (pid_t) -1) {
        INIT_SERV_ERROR(conf,
            "Not able to disociate the serv form the terminal. \n%s", "");
        return EXIT_FAILURE;
      }
      if (close(STDIN_FILENO) != 0 || close(STDOUT_FILENO) != 0
          || close(STDERR_FILENO) != 0) {
        INIT_SERV_ERROR(conf, "Wasn't able to close standart output. \n%s",
            "");
        return EXIT_FAILURE;
      }
      // création file synchroniser
      fifo_t *f = shm_fifo_init(conf->fifo_name, conf->fifo_length);
      if (f == NULL) {
        SERV_ERROR(conf, f);
        return EXIT_FAILURE;
      }
      // action du server quant au demande du client
      pid_t p;
      pthread_t t;
      while (1) {
        if ((p = shm_fifo_dequeue(f)) != (pid_t) -1) {
          // ce f ne peut pas etre libere vient
          if (pthread_create(&t, NULL, (void *(*)(void *))cmd_process,
              &p) != 0) {
            SERV_ERROR(conf, f);
            return EXIT_FAILURE;
          }
          sleep(conf->time);
        }
      }
    default:
      free(conf);
      break;
  }
  return EXIT_SUCCESS;
}

void sig_handler(int signum) {
  switch (signum) {
    case SIGTERM:
      shm_fifo_unlink(conf->fifo_name);
      kill(0, SIGKILL);
      free(conf);
      exit(EXIT_SUCCESS);
      break;
    case SIGHUP:
      // si la relecture du fichier de configuration échoue on shutdown
      //    complétement le server.
      if (process_conf_file(conf) != 0) {
        shm_fifo_unlink(conf->fifo_name);
        kill(0, SIGKILL);
        free(conf);
        exit(EXIT_FAILURE);
      }
      break;
    default:
      break;
  }
}

void *cmd_process(pid_t *p) {
  da *cmd = da_empty(sizeof(char));
  if (cmd == NULL) {
    pthread_exit(NULL);
  }
  char tubes[NUMBER_PIPE][PATH_MAX];
  for (size_t i = 0; i < NUMBER_PIPE; ++i) {
    if (snprintf(tubes[i], PATH_MAX - 1, "/tmp/%d_%zu", *p, i) <= 0) {
      da_dispose(&cmd);
      pthread_exit(NULL);
    }
  }
  int tubes_desc[NUMBER_PIPE];
  for (size_t i = 0; i < NUMBER_PIPE; ++i) {
    tubes_desc[i] = open(tubes[i], (i != 0 ? O_WRONLY : O_RDONLY));
    if (tubes_desc[i] == -1) {
      CMD_ERROR(&cmd, tubes_desc, i, "Error when trying to connect to you.\n");
      pthread_exit(NULL);
    }
  }
  char buff[BUFF_LENGTH];
  ssize_t n;
  do {
    n = read(tubes_desc[0], buff, BUFF_LENGTH);
    if (n == -1) {
      CMD_ERROR(&cmd, tubes_desc, NUMBER_PIPE,
          "Error when reading your command.\n");
      pthread_exit(NULL);
    }
    for (size_t i = 0; i < (size_t) n; ++i) {
      if (da_add(cmd, buff + i) == NULL) {
        CMD_ERROR(&cmd, tubes_desc, NUMBER_PIPE,
            "Error when reading your command. Not enougth memory.\n");
        pthread_exit(NULL);
      }
      if (buff[i] == '\0') {
        n = 0;
        break;
      }
    }
  } while (n > 0);
  //----------------------------------------------------------------------------
  size_t nb_p = 0;
  size_t nb_c = 0;
  size_t cur = 0;
  char *mcmd = da_nth(cmd, 0);
  while (mcmd[cur] != '\0') {
    if (mcmd[cur] == PIPE_TOKEN) {
      if (nb_c == 0) {
        CMD_ERROR(&cmd, tubes_desc, NUMBER_PIPE,
            "Error invalid command send.\n");
        pthread_exit(NULL);
      }
      mcmd[cur] = '\0';
      ++nb_p;
    } else if (mcmd[cur] != ' ' && (nb_c == 0 || nb_p == nb_c)) {
      ++nb_c;
    }
    ++cur;
  }
  if (nb_p >= nb_c) {
    CMD_ERROR(&cmd, tubes_desc, NUMBER_PIPE,
        "Error invalid command send.\n");
    pthread_exit(NULL);
  }
  //----------------------------------------------------------------------------
  if (nb_c > SIZE_MAX / sizeof(char **)) {
    CMD_ERROR(&cmd, tubes_desc, NUMBER_PIPE,
        "Error when reading your command. Not enougth memory.\n");
    pthread_exit(NULL);
  }
  char ***cmds = malloc(sizeof(char **) * nb_c);
  if (cmds == NULL) {
    CMD_ERROR(&cmd, tubes_desc, NUMBER_PIPE,
        "Error when reading your command. Not enougth memory.\n");
    pthread_exit(NULL);
  }
  cur = 0;
  for (size_t i = 0; i < nb_c; ++i) {
    cmds[i] = analyse_arg(mcmd + cur);
    if (cmds[i] == NULL) {
      for (size_t i = 0; i < nb_c; ++i) {
        dispose_arg(cmds[i]);
      }
      free(cmds);
      CMD_ERROR(&cmd, tubes_desc, NUMBER_PIPE,
          "Error when reading your command. Not enougth memory.\n");
      pthread_exit(NULL);
    }
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
  free(cmds);
  pthread_exit(NULL);
}

int pipe_engine(char ***cmds, size_t nmemb, int *tubes_desc) {
  if (nmemb == 0) {
    return -1;
  }
  int tub[2];
  int cin = -1;
  for (size_t i = 0; i < nmemb; ++i) {
    pid_t p;
    if (i != nmemb - 1 && pipe(tub) != 0) {
      return -1;
    }
    switch ((p = fork())) {
      case -1:
        return -1;
      case 0:
        int io = i != nmemb - 1 ? tub[1] : tubes_desc[1];
        if (i != nmemb - 1 && close(tub[0]) != 0) {
          exit(EXIT_FAILURE);
        }
        if (cin != -1 && dup2(cin, STDIN_FILENO) == -1) {
          exit(EXIT_FAILURE);
        }
        if (cin != -1 && close(cin) != 0) {
          exit(EXIT_FAILURE);
        }
        if (dup2(io, STDOUT_FILENO) == -1) {
          exit(EXIT_FAILURE);
        }
        if (close(io) != 0) {
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
        if (i == nmemb - 1) {
          if (wait(NULL) == (pid_t) -1) {
            return -1;
          }
          return 0;
        }
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
