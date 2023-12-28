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
#include "utilitaire.h"

//  BUFF_SIZE : Taille du buffer de lecture sur l'entrée standard.
#define BUFF_LENGTH 4096

// NUMBER_PIPE : nombre de tube utiliser.
#define NUMBER_PIPE 3

//  NUMBER_THREAD : nombre de thread.
#define NUMBER_THREAD 2

//	REPL : Chaine correspondant au décoration du REPL.
#define REPL ">>> "

//	CMD_SYMB : Suffixe de toutes les commandes du REPL. C'est à dire, QUIT_CMD
//		et HELP_CMD.
#define CMD_SYMB ':'
#define QUIT_CMD ":q"
#define HELP_CMD ":h"

//	HELP : Chaine correspondant à l'aide.
#define HELP "To get the help\t%s\nTo quit\t%s\n"

//	ERR_PIPE : Chaine correspondant à une erreure de pipe.
#define ERR_PIPE "Creating pipe wasn't successfull.\n%s"

//	ERR_READ : Chaine correspondant à une erreure de l'ecture.
#define ERR_READ "Reading the command wasn't successfull.\n%s"

//	ERR_REPL : Chaine correspondant à une erreur sur le REPL.
#define ERR_REPL "Error on writing/reading on the REPL.\n%s"

//	COM_ERROR : Macro fonction d'erreur de création du client.
#define COM_ERROR(reason, ...) {                                               \
		fprintf(stderr, "Error on creating communication tool. ");             \
		fprintf(stderr, (reason), __VA_ARGS__);                                \
}

//	CLIENT_ERROR : Macro fonction d'erreur de communication.
#define CLIENT_ERROR                                                           \
	fprintf(stderr, "Error on communicating with the server. ");

//	DISPOS_ALL : Macro de libération de diverse ressource du programe.
#define DISPOS_ALL(fifo, line, tubes) {                                        \
		shm_fifo_dispose((fifo));                                              \
		da_dispose(line);                                                      \
		for (size_t i = 0; i < NUMBER_PIPE; ++i) {                             \
			unlink((tubes)[i]);                                                \
		}                                                                      \
}

//	WRITE_REPL : Macro fonction d'affichage des décorations du REPL.
#define WRITE_REPL(files, f, line, tubes) {                                    \
		if (write_char_buff(STDOUT_FILENO, REPL, strlen(REPL)) != 0) {         \
			free(files);                                                       \
			DISPOS_ALL(&(f), line, tubes);                                     \
			CLIENT_ERROR;                                                      \
			return EXIT_FAILURE;                                               \
		}                                                                      \
}

//	file_t : structure représentant les
typedef struct {
	int in;
	int out;
} file_t;

//	write_char : Ecrit dans le descritpeur de fichier desc les éléments de buff
//		supposer être des caractères.
//		Renvoie 0 en cas de succés, une valeur négatif dans le cas contraitre.
static int write_char(int desc, da *buff);

//	read_write : Lis ce qui est présent sur l'entréer pointé par f->in puis
//		l'écrit dans f->out.
//		Renvoie NULL en cas d'erreur. Sinon renvoie f.
static void *read_write(file_t *f);

int main(void) {
	//  récupération des information du fichier de configuration.
	conf_t conf;
	if (process_conf_file(&conf) != 0) {
		return EXIT_FAILURE;
	}
	// tableau contenant le nom des tubes de communication vers le server
	char tubes[NUMBER_PIPE][PATH_MAX];
	for (size_t i = 0; i < NUMBER_PIPE; ++i) {
		if (snprintf(tubes[i], PATH_MAX - 1, "/tmp/%d_%zu", getpid(), i) <= 0) {
			COM_ERROR(ERR_PIPE, "");
			return EXIT_FAILURE;
		}
	}
	// création des tubes de communication.
	if (mkfifo(tubes[0], S_IRUSR | S_IWUSR) != 0) {
		COM_ERROR(ERR_PIPE, "");
		return EXIT_FAILURE;
	}
	if (mkfifo(tubes[1], S_IRUSR | S_IWUSR) != 0) {
		COM_ERROR(ERR_PIPE, "");
		return EXIT_FAILURE;
	}
	if (mkfifo(tubes[2], S_IRUSR | S_IWUSR) != 0) {
		COM_ERROR(ERR_PIPE, "");
		unlink(tubes[0]);
		unlink(tubes[1]);
		return EXIT_FAILURE;
	}
	// ouverture de la file synchroniser.
	fifo_t *f = shm_fifo_open(conf.fifo_name, conf.fifo_length);
	if (f == NULL) {
		for (size_t i = 0; i < NUMBER_PIPE; ++i) {
			unlink(tubes[i]);
		}
		COM_ERROR(ERR_MEMORY, "");
		return EXIT_FAILURE;
	}
	//  création des buffers de lecture.
	ssize_t n;
	char buff[BUFF_LENGTH];
	file_t *files = NULL;
	da *line = da_empty(sizeof(char));
	if (line == NULL) {
		DISPOS_ALL(&f, &line, tubes);
		COM_ERROR(ERR_MEMORY, "");
		return EXIT_FAILURE;
	}
	int tubes_desc[NUMBER_PIPE] = { -1 };
	pthread_t threads[NUMBER_THREAD];
	files = malloc(sizeof *files * NUMBER_THREAD);
	if (files == NULL) {
		DISPOS_ALL(&f, &line, tubes);
		COM_ERROR(ERR_MEMORY, "");
		return EXIT_FAILURE;
	}
	// ecriture des décoration du REPL.
	WRITE_REPL(files, f, &line, tubes);
	// boucle de lecture
	do {
		n = read(STDIN_FILENO, buff, BUFF_LENGTH);
		if (n == -1) {
			free(files);
			DISPOS_ALL(&f, &line, tubes);
			CLIENT_ERROR;
			return EXIT_FAILURE;
		}
		for (size_t i = 0; i < (size_t) n; ++i) {
			if (*(buff + i) != '\n') {
				if (da_add(line, buff + i) == NULL) {
					free(files);
					DISPOS_ALL(&f, &line, tubes);
					CLIENT_ERROR;
					return EXIT_FAILURE;
				}
				continue;
			}
			char t = '\0';
			if (da_add(line, &t) == NULL) {
				free(files);
				DISPOS_ALL(&f, &line, tubes);
				CLIENT_ERROR;
				return EXIT_FAILURE;
			}
			// Gestion des commandes du REPL.
			const char *p = da_nth(line, 0);
			if (*p == CMD_SYMB) {
				if (is_prefixe(p, QUIT_CMD) == 0) {
					free(files);
					DISPOS_ALL(&f, &line, tubes);
					return EXIT_SUCCESS;
				} else if (is_prefixe(p, HELP_CMD) == 0) {
					da_reset(line);
					printf(HELP, HELP_CMD, QUIT_CMD);
					WRITE_REPL(files, f, &line, tubes);
					continue;
				} else {
					fprintf(stderr, "The '%s' command is unknow. Check the '%s'"
					        " command for a list of all command.\n", p,
					        HELP_CMD);
					free(files);
					DISPOS_ALL(&f, &line, tubes);
					return EXIT_FAILURE;
				}
			}
			// On effetue une demande au server pour être traiter.
			if (shm_fifo_enqueue(f, getpid()) != 0) {
				free(files);
				DISPOS_ALL(&f, &line, tubes);
				CLIENT_ERROR;
				return EXIT_FAILURE;
			}
			for (size_t i = 0; i < NUMBER_PIPE; ++i) {
				tubes_desc[i] = open(tubes[i], (i != 0 ? O_RDONLY : O_WRONLY));
				if (tubes_desc[i] == -1) {
					free(files);
					DISPOS_ALL(&f, &line, tubes);
					CLIENT_ERROR;
					return EXIT_FAILURE;
				}
			}
			files[0].in = tubes_desc[1];
			files[0].out = STDOUT_FILENO;
			files[1].in = tubes_desc[2];
			files[1].out = STDERR_FILENO;
			// On écoute les futurs réponse du server.
			for (size_t i = 0; i < NUMBER_THREAD; ++i) {
				if (pthread_create(threads + i, NULL, (void *(*)(void *))
				                   read_write, files + i) != 0) {
					free(files);
					DISPOS_ALL(&f, &line, tubes);
					CLIENT_ERROR;
					return EXIT_FAILURE;
				}
			}
			// On envoie la commande au server
			if (write_char(tubes_desc[0], line) != 0) {
				free(files);
				DISPOS_ALL(&f, &line, tubes);
				CLIENT_ERROR;
				return EXIT_FAILURE;
			}
			// On attend la réponse du server.
			for (size_t i = 0; i < NUMBER_THREAD; ++i) {
				file_t *v;
				if (pthread_join(threads[i], (void *) &v) != 0 || v == NULL) {
					free(files);
					DISPOS_ALL(&f, &line, tubes);
					CLIENT_ERROR;
					return EXIT_FAILURE;
				}
			}
			// On ferme les tubes utiliser.
			for (size_t i = 0; i < NUMBER_PIPE; ++i) {
				if (close(tubes_desc[i]) != 0) {
					free(files);
					DISPOS_ALL(&f, &line, tubes);
					CLIENT_ERROR;
					return EXIT_FAILURE;
				}
			}
			WRITE_REPL(files, f, &line, tubes);
			da_reset(line);
		}
	} while (n > 0);
	free(files);
	DISPOS_ALL(&f, &line, tubes);
	return EXIT_SUCCESS;
}

int write_char(int desc, da *buff) {
	size_t nb_write = 0;
	while (nb_write != da_length(buff)) {
		ssize_t n = write(desc, da_nth(buff, nb_write),
		                  da_length(buff) - nb_write);
		if (n == -1) {
			return -1;
		}
		nb_write += (size_t) n;
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
